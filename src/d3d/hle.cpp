// The title's Direct3D calls as state for the renderer (docs/renderer.md). Ported from
// SCDA-XBERecomp's xdk/d3d8.cpp (see NOTICE) to Ninja Gaiden Black's XDK 5849 D3D8LTCG build:
// its own calling conventions (src/d3d/d3d_funcs.h), vertex programs loaded into the GPU's
// program memory (LoadVertexShader / SelectVertexShader), and pre-recorded index push buffers
// (RunPushBuffer).
//
// NGB_RENDERER=mirror: every call still goes to the recompiled Direct3D (xemu draws and shows the
// game); this keeps a copy of the state and draws it with the renderer as well, for comparing.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "d3d_funcs.h"
#include "gpu/backend.h"
#include "gpu/state.h"
#include "gpu/texture.h"
#include "gpu/vertex.h"
#include "kernel/kernel.h"

extern "C" uint32_t xbox_HeapAlloc(uint32_t size, uint32_t align);

using kernel::HostPtr;
using kernel::R32;
using kernel::W32;

namespace {

enum Reg { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };

// the call: registers as the hook saw them, stack arguments above the return address
struct Call {
    uint32_t* r;
    uint32_t a(int i) const { return R32(r[ESP] + 4 + 4 * i); }
    float af(int i) const { uint32_t v = a(i); float f; std::memcpy(&f, &v, 4); return f; }
};

// Direct3D's globals in this build (XbSymbolDatabase's names)
constexpr uint32_t kRenderState = 0x3D83D8;      // D3D__RenderState[166]
constexpr uint32_t kTextureState = 0x3D5D10;     // D3D__TextureState[4][32]
constexpr uint32_t kDevicePtr = 0x3D83C8;       // D3D__pDevice
constexpr uint32_t kVsConstants = 0x3D8678;      // the device's copy of c[0..191], hardware order
constexpr uint32_t kSimpleEncode = 0x3E9278;     // D3D__SimpleRenderStateEncode[92]: each simple state's method

gpu::State g_state;
std::recursive_mutex g_lock;
std::unique_ptr<gpu::Backend> g_backend;
uint32_t g_vis_index;   // D3DDevice_EndVisibilityTest's index (in eax), for its report slot after the body ran
bool g_mirror = true;

std::unordered_map<uint32_t, std::shared_ptr<gpu::VertexShader>> g_vs;
std::unordered_map<uint32_t, std::shared_ptr<gpu::PixelShader>> g_ps;

// CreateVertexShader / CreatePixelShader: the arguments, until the recompiled body wrote the handle
uint32_t g_new_decl, g_new_func, g_new_out;

// the GPU's vertex program memory, 136 instructions of 4 dwords, as LoadVertexShader fills it
uint32_t g_program[136][4];
std::shared_ptr<gpu::VertexShader> g_selected;   // SelectVertexShader's program and declaration

void SyncDeferred() {
    // deferred states live in the title's memory, which the inline SetRenderState and
    // SetTextureStageState write; so do the vertex shader constants
    // deferred and complex alike: the XDK keeps every one in D3D__RenderState, and inlined code
    // writes there without any function of ours seeing it
    for (uint32_t i = gpu::kRsDeferredFirst; i < gpu::kRsMax; i++) g_state.rs[i] = R32(kRenderState + i * 4);
    // the simple states too, as SetRenderState keeps them (the device's defaults were set before
    // the hooks saw anything); not the pixel shader's, which SetPixelShader loads from its definition
    for (uint32_t i = 0; i < gpu::kRsDeferredFirst; i++) {
        uint32_t method = R32(kSimpleEncode + i * 4) & 0x1FFC;
        if ((method >= 0x300 && method < 0x400) || method == 0x1D78)
            g_state.simple[method >> 2] = R32(kRenderState + i * 4);
    }
    for (int s = 0; s < 4; s++)
        for (uint32_t i = 0; i < gpu::kTssBumpEnvMat00; i++) g_state.tss[s][i] = R32(kTextureState + (s * 32 + i) * 4);
    std::memcpy(g_state.vsc.data(), HostPtr(kVsConstants), sizeof(g_state.vsc));
    // fixed function: the transforms (view, projection, texture 0-3, world 0-3 at +0xC60) and the
    // material (+0xF00) are written into the device by inlined code
    uint32_t dev = R32(kDevicePtr);
    if (dev) {
        for (uint32_t t = 0; t < 10; t++) {
            uint32_t pc = t < 2 ? t + 2 : t < 6 ? t + 14 : t + 250;
            std::memcpy(g_state.transforms[pc].data(), HostPtr(dev + 0xC60 + t * 64), 64);
        }
        std::memcpy(g_state.material.data(), HostPtr(dev + 0xF00), 17 * 4);
    }
}

void UpdateViewportConstants() {
    auto& v = g_state.viewport;
    float hw = v.w * 0.5f, hh = v.h * 0.5f;
    g_state.vsc[58] = {hw, -hh, (v.max_z - v.min_z) * g_state.zscale, 0};
    g_state.vsc[59] = {v.x + g_state.ss_offset_x + hw, v.y + g_state.ss_offset_y + hh, v.min_z * g_state.zscale, 0};
}

gpu::Surface SurfaceOf(uint32_t header) {
    gpu::Surface s;
    s.header = header;
    if (header) {
        s.data = R32(header + 4);
        s.format = R32(header + 12);
        s.size = R32(header + 16);
    }
    return s;
}

std::shared_ptr<gpu::VertexShader> ReadVertexShader(uint32_t decl, uint32_t func) {
    auto vs = std::make_shared<gpu::VertexShader>();
    if (decl)
        for (uint32_t i = 0; i < 1024; i++) {
            uint32_t t = R32(decl + i * 4);
            vs->decl.push_back(t);
            if (t == 0xFFFFFFFF) break;
        }
    if (func) {
        uint32_t hdr = R32(func);
        vs->program_type = hdr & 0xFFFF;
        uint32_t n = hdr >> 16;
        for (uint32_t i = 0; i < n * 4; i++) vs->program.push_back(R32(func + 4 + i * 4));
        if (std::getenv("NGB_D3D_VPLOG"))   // debugging: where the vertex programs come from
            std::fprintf(stderr, "[VP] func %08X n %u first %08X %08X %08X %08X\n", func, n, n ? vs->program[0] : 0,
                         n ? vs->program[1] : 0, n ? vs->program[2] : 0, n ? vs->program[3] : 0);
    }
    gpu::ParseDeclaration(*vs);
    return vs;
}

void Draw(uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) {
    SyncDeferred();
    static bool nolight = std::getenv("NGB_D3D_NOLIGHT") != nullptr;   // debugging
    if (nolight) g_state.rs[gpu::kRsLighting] = 0;
    static bool nofog = std::getenv("NGB_D3D_NOFOG") != nullptr;       // debugging
    if (nofog) g_state.rs[gpu::kRsFogEnable] = 0;
    static int fogn = std::getenv("NGB_D3D_FOGLOG") ? 1 : 0;
    static uint32_t fog_last[5];
    uint32_t fog_now[5] = { g_state.rs[gpu::kRsFogEnable], g_state.rs[gpu::kRsFogStart], g_state.rs[gpu::kRsFogEnd],
                            g_state.rs[gpu::kRsFogColor], g_state.rs[gpu::kRsFogTableMode] };
    if (fogn > 0 && std::memcmp(fog_now, fog_last, sizeof fog_now)) {
        std::memcpy(fog_last, fog_now, sizeof fog_now);
        float st, en, de;
        std::memcpy(&st, &g_state.rs[gpu::kRsFogStart], 4);
        std::memcpy(&en, &g_state.rs[gpu::kRsFogEnd], 4);
        std::memcpy(&de, &g_state.rs[gpu::kRsFogDensity], 4);
        std::fprintf(stderr, "[D3D] fog table %u start %g end %g density %g color %08X range %u\n",
                     g_state.rs[gpu::kRsFogTableMode], st, en, de, g_state.rs[gpu::kRsFogColor],
                     g_state.rs[gpu::kRsRangeFogEnable]);
    }
    UpdateViewportConstants();
    if (g_backend) g_backend->Draw(g_state, prim, start, count, indices);
}

// A pushed index list (RunPushBuffer): BEGIN (0x17FC) with the primitive, then 16-bit index
// pairs (0x1800) or 32-bit indices (0x1808), END (BEGIN 0). Anything else is reported once.
void RunPushBuffer(uint32_t pb) {
    uint32_t data = R32(pb + 4) | 0x80000000u, size = R32(pb + 0xC);
    std::vector<uint16_t> idx;
    uint32_t prim = 0;
    for (uint32_t o = 0; o + 4 <= size;) {
        uint32_t h = R32(data + o), count = (h >> 18) & 0x7FF, method = h & 0x1FFC;
        if ((h & 0xE0000003) == 0x20000000 || (h & 3) == 1) break;   // jump or call: not expected
        const uint32_t* v = static_cast<const uint32_t*>(HostPtr(data + o + 4));
        if (method == 0x17FC) {
            if (v[0]) {
                prim = v[0];
                idx.clear();
            } else if (prim) {
                Draw(prim, 0, uint32_t(idx.size()), reinterpret_cast<const uint8_t*>(idx.data()));
                prim = 0;
            }
        } else if (method == 0x1800) {
            for (uint32_t i = 0; i < count; i++) {
                idx.push_back(uint16_t(v[i]));
                idx.push_back(uint16_t(v[i] >> 16));
            }
        } else if (method == 0x1808) {
            for (uint32_t i = 0; i < count; i++) idx.push_back(uint16_t(v[i]));
        } else {
            static uint32_t seen[64];
            static int n;
            bool known = false;
            for (int i = 0; i < n; i++) known |= seen[i] == method;
            if (!known && n < 64) {
                seen[n++] = method;
                std::fprintf(stderr, "[D3D] push buffer method %04X x%u not drawn\n", method, count);
            }
        }
        o += 4 + 4 * count;
    }
    // a buffer that ends without END still draws what it began
    if (prim && !idx.empty()) Draw(prim, 0, uint32_t(idx.size()), reinterpret_cast<const uint8_t*>(idx.data()));
}

// before the call: false to let the recompiled body run (always, mirroring)
bool Before(int fn, Call c) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    switch (fn) {
    // render states
    case D3DF_D3DDevice_SetRenderState_Simple:
        g_state.simple[(c.r[ECX] & 0x1FFC) >> 2] = c.r[EDX];
        break;
#define COMPLEX(name, index) case D3DF_D3DDevice_SetRenderState_##name: g_state.rs[index] = c.a(0); break;
    COMPLEX(PSTextureModes, gpu::kRsPsTextureModes)
    COMPLEX(VertexBlend, gpu::kRsVertexBlend)
    COMPLEX(FogColor, gpu::kRsFogColor)
    COMPLEX(FillMode, gpu::kRsFillMode)
    COMPLEX(BackFillMode, gpu::kRsBackFillMode)
    COMPLEX(TwoSidedLighting, gpu::kRsTwoSidedLighting)
    COMPLEX(NormalizeNormals, gpu::kRsNormalizeNormals)
    COMPLEX(ZEnable, gpu::kRsZEnable)
    COMPLEX(StencilEnable, gpu::kRsStencilEnable)
    COMPLEX(StencilFail, gpu::kRsStencilFail)
    COMPLEX(FrontFace, gpu::kRsFrontFace)
    COMPLEX(CullMode, gpu::kRsCullMode)
    COMPLEX(TextureFactor, gpu::kRsTextureFactor)
    COMPLEX(LogicOp, gpu::kRsLogicOp)
    COMPLEX(EdgeAntiAlias, gpu::kRsEdgeAntiAlias)
    COMPLEX(MultiSampleAntiAlias, gpu::kRsMultiSampleAntiAlias)
    COMPLEX(MultiSampleMask, gpu::kRsMultiSampleMask)
    COMPLEX(MultiSampleMode, gpu::kRsMultiSampleMode)
    COMPLEX(MultiSampleRenderTargetMode, gpu::kRsMultiSampleRenderTargetMode)
    COMPLEX(ShadowFunc, gpu::kRsShadowFunc)
    COMPLEX(LineWidth, gpu::kRsLineWidth)
    COMPLEX(SampleAlpha, gpu::kRsSampleAlpha)
    COMPLEX(Dxt1NoiseEnable, gpu::kRsDxt1NoiseEnable)
    COMPLEX(YuvEnable, gpu::kRsYuvEnable)
    COMPLEX(OcclusionCullEnable, gpu::kRsOcclusionCullEnable)
    COMPLEX(StencilCullEnable, gpu::kRsStencilCullEnable)
    COMPLEX(RopZCmpAlwaysRead, gpu::kRsRopZCmpAlwaysRead)
    COMPLEX(RopZRead, gpu::kRsRopZRead)
    COMPLEX(DoNotCullUncompressed, gpu::kRsDoNotCullUncompressed)
#undef COMPLEX

    // texture stage states: (stage eax, type edx, value ecx), BumpEnv (stage eax; type, value),
    // TexCoordIndex (stage esi; value), BorderColor and ColorKeyColor (stage eax, value ebx)
    case D3DF_D3DDevice_SetTextureStageStateNotInline:
        if (c.r[EDX] < gpu::kTssCount) g_state.tss[c.r[EAX] & 3][c.r[EDX]] = c.r[ECX];
        break;
    case D3DF_D3DDevice_SetTextureState_BumpEnv:
        if (c.a(0) >= gpu::kTssBumpEnvMat00 && c.a(0) <= gpu::kTssBumpEnvLOffset)
            g_state.tss[c.r[EAX] & 3][c.a(0)] = c.a(1);
        break;
    case D3DF_D3DDevice_SetTextureState_TexCoordIndex:
        g_state.tss[c.r[ESI] & 3][gpu::kTssTexCoordIndex] = c.a(0);
        break;
    case D3DF_D3DDevice_SetTextureState_BorderColor:
        g_state.tss[c.r[EAX] & 3][gpu::kTssBorderColor] = c.r[EBX];
        break;
    case D3DF_D3DDevice_SetTextureState_ColorKeyColor:
        g_state.tss[c.r[EAX] & 3][gpu::kTssColorKeyColor] = c.r[EBX];
        break;

    // fixed function: SetTransform (type eax, matrix edx), SetLight (index, light), LightEnable
    // (index eax; on)
    case D3DF_D3DDevice_SetTransform: {
        uint32_t t = c.r[EAX];
        if (t < 10) {
            uint32_t pc = t < 2 ? t + 2 : t < 6 ? t + 14 : t + 250;
            std::memcpy(g_state.transforms[pc].data(), HostPtr(c.r[EDX]), 64);
        }
        break;
    }
    case D3DF_D3DDevice_SetLight:
        if (c.a(0) < 8 && c.a(1)) std::memcpy(g_state.lights[c.a(0)].data(), HostPtr(c.a(1)), 26 * 4);
        break;
    case D3DF_D3DDevice_LightEnable:
        if (c.r[EAX] < 8)
            g_state.light_enable = c.a(0) ? g_state.light_enable | 1u << c.r[EAX] : g_state.light_enable & ~(1u << c.r[EAX]);
        break;

    // resources: SetTexture (stage eax; texture), SetPalette (stage eax; palette), SetStreamSource
    // (stream eax, buffer ebx; stride)
    case D3DF_D3DDevice_SetTexture:
        g_state.textures[c.r[EAX] & 3] = c.a(0);
        break;
    case D3DF_D3DDevice_SetPalette:
        g_state.palettes[c.r[EAX] & 3] = c.a(0);
        break;
    case D3DF_D3DDevice_SetStreamSource:
        g_state.streams[c.r[EAX] & 15] = {c.r[EBX], c.a(0)};
        break;

    // shaders
    case D3DF_D3DDevice_CreateVertexShader:
        g_new_decl = c.a(0);
        g_new_func = c.a(1);
        g_new_out = c.a(2);
        break;
    case D3DF_D3DDevice_CreatePixelShader:
        g_new_decl = c.a(0);
        g_new_out = c.a(1);
        break;
    case D3DF_D3DDevice_SetVertexShader: {
        uint32_t h = c.a(0);
        g_state.vs_handle = h;
        g_state.vs = nullptr;
        if (h & 1) {
            auto it = g_vs.find(h);
            if (it != g_vs.end()) g_state.vs = it->second;
        }
        break;
    }
    case D3DF_D3DDevice_LoadVertexShader: {          // (handle eax; address): its program into memory
        auto it = g_vs.find(c.r[EAX]);
        uint32_t at = c.a(0);
        if (it != g_vs.end())
            for (size_t i = 0; i < it->second->program.size() / 4 && at + i < 136; i++)
                std::memcpy(g_program[at + i], &it->second->program[i * 4], 16);
        break;
    }
    case D3DF_D3DDevice_SelectVertexShader: {        // (handle eax, address ebx): run from there
        uint32_t h = c.r[EAX], at = c.r[EBX];
        auto vs = std::make_shared<gpu::VertexShader>();
        auto it = g_vs.find(h);
        if (it != g_vs.end()) {
            vs->decl = it->second->decl;
            vs->inputs = it->second->inputs;
            vs->stream_stride = it->second->stream_stride;
        }
        vs->program_type = 0x2078;
        for (uint32_t i = at; i < 136; i++) {
            vs->program.insert(vs->program.end(), g_program[i], g_program[i] + 4);
            if (g_program[i][3] & 1) break;          // the final instruction
        }
        vs->handle = h;
        g_selected = vs;
        g_state.vs_handle = h | 1;
        g_state.vs = vs;
        break;
    }
    case D3DF_D3DDevice_SetPixelShader: {
        // the handle is the PixelShader object: 12 bytes of header, then its copy of the
        // definition (CreatePixelShader allocates 0xFC and copies 0x3C dwords to +0xC); the
        // game made most of its shaders before the hooks recorded any, so read it from there
        g_state.ps = nullptr;
        uint32_t h = c.r[EAX];
        if (!h) break;
        const uint32_t* def = static_cast<const uint32_t*>(HostPtr(h + 0xC));
        auto& ps = g_ps[h];
        if (!ps || std::memcmp(ps->def, def, sizeof(ps->def)) != 0) {
            ps = std::make_shared<gpu::PixelShader>();
            std::memcpy(ps->def, def, sizeof(ps->def));
            ps->handle = h;
        }
        g_state.ps = ps;
        const uint32_t* d = ps->def;
        auto put = [&](uint32_t method, uint32_t v) { g_state.simple[method >> 2] = v; };
        for (int i = 0; i < 8; i++) {
            put(gpu::kMPsAlphaInputs + i * 4, d[0 + i]);
            put(gpu::kMPsConstant0 + i * 4, d[10 + i]);
            put(gpu::kMPsConstant1 + i * 4, d[18 + i]);
            put(gpu::kMPsAlphaOutputs + i * 4, d[26 + i]);
            put(gpu::kMPsRgbInputs + i * 4, d[34 + i]);
            put(gpu::kMPsRgbOutputs + i * 4, d[45 + i]);
        }
        put(gpu::kMPsFinalAbcd, d[8]);
        put(gpu::kMPsFinalEfgh, d[9]);
        put(gpu::kMPsCompareMode, d[42]);
        put(gpu::kMPsFinalConstant0, d[43]);
        put(gpu::kMPsFinalConstant1, d[44]);
        put(gpu::kMPsCombinerCount, d[53]);
        put(gpu::kMPsDotMapping, d[55]);
        put(gpu::kMPsInputTexture, d[56]);
        g_state.rs[gpu::kRsPsTextureModes] = d[54];
        break;
    }
    case D3DF_D3DDevice_SetPixelShaderConstant: {    // (register ecx; source; count eax)
        uint32_t reg = c.r[ECX], src = c.a(0), n = c.r[EAX];
        for (uint32_t i = 0; i < n && reg + i < 16; i++) std::memcpy(g_state.psc[reg + i].data(), HostPtr(src + i * 16), 16);
        gpu::ApplyPixelConstants(g_state, reg, n);
        break;
    }

    // targets and the viewport
    case D3DF_D3DDevice_SetRenderTarget: {
        if (c.a(0)) g_state.rt = SurfaceOf(c.a(0));
        g_state.zs = SurfaceOf(c.a(1));
        uint32_t w, h;
        gpu::SurfaceSize(g_state.rt, w, h);
        static int rtlog = std::getenv("NGB_D3D_RTLOG") ? 400 : 0;   // debugging: the targets as they are set
        if (rtlog > 0) {
            uint32_t zw = 0, zh = 0;
            gpu::SurfaceSize(g_state.zs, zw, zh);
            if (!c.a(0) || zw != w || zh != h) {
                rtlog--;
                std::fprintf(stderr, "[RT] SetRenderTarget rt %08X (data %08X fmt %08X %ux%u) zs %08X (data %08X fmt %08X %ux%u)\n",
                             c.a(0), g_state.rt.data, g_state.rt.format, w, h, c.a(1), g_state.zs.data, g_state.zs.format, zw, zh);
            }
        }
        g_state.viewport = {0, 0, w, h, 0.0f, 1.0f};
        uint32_t f = g_state.zs.header ? (g_state.zs.format >> 8) & 0xFF : 0x2E;
        g_state.zscale = f == 0x2C || f == 0x30 ? 65535.0f : 16777215.0f;
        break;
    }
    case D3DF_D3DDevice_SetViewport: {
        uint32_t v = c.a(0);
        float mn, mx;
        std::memcpy(&mn, HostPtr(v + 16), 4);
        std::memcpy(&mx, HostPtr(v + 20), 4);
        g_state.viewport = {R32(v), R32(v + 4), R32(v + 8), R32(v + 12), mn, mx};
        static int vplog = std::getenv("NGB_D3D_RTLOG") ? 400 : 0;
        if (vplog > 0 && (g_state.viewport.w <= 256 || !g_state.rt.data)) {
            vplog--;
            std::fprintf(stderr, "[RT] SetViewport %u %u %u %u z %g..%g (rt data %08X)\n", g_state.viewport.x, g_state.viewport.y,
                         g_state.viewport.w, g_state.viewport.h, mn, mx, g_state.rt.data);
        }
        // clamped to the target, as the XDK does: SetRenderTarget sets a 0x7FFFFFFF square
        uint32_t w, h;
        gpu::SurfaceSize(g_state.rt, w, h);
        if (w && h) {
            g_state.viewport.x = std::min(g_state.viewport.x, w);
            g_state.viewport.y = std::min(g_state.viewport.y, h);
            g_state.viewport.w = std::min(g_state.viewport.w, w - g_state.viewport.x);
            g_state.viewport.h = std::min(g_state.viewport.h, h - g_state.viewport.y);
        }
        g_state.scissor_count = 0;
        break;
    }
    case D3DF_D3DDevice_SetScissors:
        g_state.scissor_count = std::min<uint32_t>(c.a(0), 8);
        for (uint32_t i = 0; i < g_state.scissor_count * 4; i++) g_state.scissors[i] = R32(c.a(2) + i * 4);
        break;
    case D3DF_D3DDevice_SetScreenSpaceOffset:
        g_state.ss_offset_x = c.af(0);
        g_state.ss_offset_y = c.af(1);
        if (std::getenv("NGB_D3D_SSLOG")) {
            static float lx = -1e9f, ly = -1e9f;
            if (g_state.ss_offset_x != lx || g_state.ss_offset_y != ly)
                std::fprintf(stderr, "[D3D] screen space offset %g %g\n", g_state.ss_offset_x, g_state.ss_offset_y);
            lx = g_state.ss_offset_x;
            ly = g_state.ss_offset_y;
        }
        break;

    // drawing: DrawVerticesUP (prim, count, data, stride), DrawIndexedVertices (prim, count,
    // indices), DrawVertices (prim; start ecx, count eax), RunPushBuffer (buffer, fixups)
    case D3DF_D3DDevice_DrawVerticesUP: {
        uint32_t prim = c.a(0), count = c.a(1), data = c.a(2), stride = c.a(3);
        if (!count || !stride || !g_backend) break;
        // stream 0 points at the vertices through a vertex buffer header of ours, its data
        // address a virtual one (Common bit 31, mem::ResourceData)
        auto saved = g_state.streams[0];
        static uint32_t guest_header;
        if (!guest_header) guest_header = xbox_HeapAlloc(20, 16);
        W32(guest_header, 0x80000001u);
        W32(guest_header + 4, data);
        g_state.streams[0] = {guest_header, stride};
        Draw(prim, 0, count, nullptr);
        g_state.streams[0] = saved;
        break;
    }
    case D3DF_D3DDevice_DrawIndexedVertices:
        if (c.a(2)) Draw(c.a(0), 0, c.a(1), static_cast<const uint8_t*>(HostPtr(c.a(2))));
        break;
    case D3DF_D3DDevice_DrawVertices:
        Draw(c.a(0), c.r[ECX], c.r[EAX], nullptr);
        break;
    case D3DF_D3DDevice_RunPushBuffer:
        RunPushBuffer(c.a(0));
        break;

    case D3DF_D3DDevice_Clear: {                     // (count, rects, flags, color, z, stencil)
        if (!g_backend) break;
        const uint8_t* rects = c.a(0) && c.a(1) ? static_cast<const uint8_t*>(HostPtr(c.a(1))) : nullptr;
        g_backend->Clear(g_state, c.a(0), rects, c.a(2), c.a(3), c.af(4), c.a(5));
        break;
    }
    case D3DF_D3DDevice_CopyRects:                   // (source, rects, count, destination, points)
        if (g_backend) g_backend->Copy(SurfaceOf(c.a(0)), SurfaceOf(c.a(3)), c.a(2), c.a(1), c.a(4));
        break;
    case D3DF_D3DDevice_Swap:
        if (std::getenv("NGB_ASPECT_LOG")) {   // debugging: the projection's aspect (m22 / m11)
            static unsigned n;
            const auto& m = g_state.transforms[3];
            float m11, m22;
            std::memcpy(&m11, &m[0], 4);
            std::memcpy(&m22, &m[5], 4);
            if (n++ % 120 == 0) std::fprintf(stderr, "[ASPECT] projection m11 %.4f m22 %.4f aspect %.4f\n", m11, m22, m22 / m11);
        }
        if (g_backend) g_backend->Present(g_state.rt);
        break;
    case D3DF_D3DDevice_SetGammaRamp:
        if (g_backend) g_backend->SetGamma(static_cast<const uint8_t*>(HostPtr(c.a(1))));
        break;
    case D3DF_D3DDevice_EndVisibilityTest:
        g_vis_index = c.r[EAX];
        break;
    default:
        break;
    }
    return false;
}

// after the recompiled body: what it made
void After(int fn, Call c) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    switch (fn) {
    case D3DF_D3DDevice_CreateVertexShader: {
        uint32_t h = g_new_out ? R32(g_new_out) : 0;
        if (h) {
            auto vs = ReadVertexShader(g_new_decl, g_new_func);
            vs->handle = h;
            g_vs[h] = vs;
        }
        break;
    }
    case D3DF_D3DDevice_CreatePixelShader: {
        uint32_t h = g_new_out ? R32(g_new_out) : 0;
        if (h && g_new_decl) {
            auto ps = std::make_shared<gpu::PixelShader>();
            std::memcpy(ps->def, HostPtr(g_new_decl), sizeof(ps->def));
            ps->handle = h;
            g_ps[h] = ps;
        }
        break;
    }
    case D3DF_D3DDevice_SetRenderTarget:             // the default target when the argument was 0
        break;
    case D3DF_D3DDevice_EndVisibilityTest: {
        // its report slot: 16 bytes in the device's pages of them (allocated by the body as needed), where the
        // GPU writes the count; the renderer writes it there instead, once the frame's draws are done
        uint32_t dev = R32(kDevicePtr), page = dev ? R32(dev + 0x7D4 + (g_vis_index >> 8) * 4) : 0;
        if (g_backend && !g_mirror && page) g_backend->VisibilityEnd(page + (g_vis_index & 0xFF) * 16);
        break;
    }
    case D3DF_D3DDevice_Swap:
        // the swap turns the back buffer's surface to the other buffer's memory: the frame's first
        // clear (made before any SetRenderTarget) goes there, not into the picture just shown -- the
        // picture a cut's cross-fade copies at the next frame
        if (g_state.rt.header) g_state.rt = SurfaceOf(g_state.rt.header);
        break;
    default:
        break;
    }
}

extern "C" [[noreturn]] void ngb_renderer_fatal(const char* detail);  // src/ingame_menu.c

bool g_started;

void Start() {
    g_started = true;
    const char* e = std::getenv("NGB_RENDERER");
    g_mirror = !e || std::strcmp(e, "d3d") != 0;
    g_state = gpu::State{};
    g_backend = gpu::CreateBackend();
    if (g_backend && !g_backend->Init(640, 480)) {
        g_backend.reset();
        if (gpu::g_init_retry) {   // the NV2A renderer's device is not up yet: at the next call
            gpu::g_init_retry = false;
            g_started = false;
            return;
        }
        std::fprintf(stderr, "[D3D] renderer init failed\n");
        if (!g_mirror) ngb_renderer_fatal("The renderer could not start on this GPU (details above in the log).");
    }
    std::fprintf(stderr, "[D3D] renderer on (%s)\n", g_mirror ? "mirroring the NV2A emulation" : "drawing to the window");
}

}  // namespace

extern "C" int d3d_hle_call(int fn, uint32_t regs[8]) {
    if (!g_started) Start();
    return Before(fn, Call{regs}) ? 1 : 0;
}

// D3DDevice_BeginVisibilityTest, inlined into the title (src/d3d/d3d_hooks.c)
extern "C" void d3d_visibility_begin(void) {
    if (!g_started) Start();
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    if (g_backend && !g_mirror) g_backend->VisibilityBegin();
}

extern "C" void d3d_hle_after(int fn, const uint32_t regs[8]) {
    if (!g_started) Start();
    After(fn, Call{const_cast<uint32_t*>(regs)});
}
