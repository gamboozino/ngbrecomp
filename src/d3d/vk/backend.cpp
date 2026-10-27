// The renderer on Vulkan (docs/renderer.md): the state the title handed Direct3D (gpu/state.h)
// drawn with shaders gpu/shaders.cpp writes. Modelled on SCDA-XBERecomp's gpu/d3d12.cpp (see
// NOTICE), without its Splinter Cell specifics (HDR, ambient occlusion, SMAA).
//
// Everything runs on the calling (game) thread and is submitted in one command buffer per frame,
// waited for at Present. Render targets live on the GPU, keyed by their guest memory; textures
// are decoded from guest memory and kept while their bytes are the same.
//
// NGB_D3D_DUMP=dir writes each presented frame (and NGB_D3D_DUMP_FROM=n, NGB_D3D_DUMP_COUNT=n
// choose which) as dir/frame_NNNNN.ppm.
#include "port.h"
#include <volk.h>
#include <vk_mem_alloc.h>

#include <algorithm>
#include <bit>
#include <map>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <sstream>
#include <iomanip>
#include <memory>
#include <set>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <deque>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "gpu/backend.h"
#include "gpu/shaders.h"
#include "gpu/texture.h"
#include "gpu/vertex.h"
#include "gpu/headers.h"
#include "kernel/kernel.h"
#include "vk/spirv.h"
#include "nv2a_vk_share.h"   // the NV2A renderer's device and presenter (NGB_RENDERER=d3d)
extern "C" {
#include "quick_select.h"
int ngb_pc_setting(int which);
}

extern "C" volatile int ngb_nv2a_logdraws;   // xemu pgraph/vk/draw.c

namespace gpu {
namespace {

using kernel::HostPtr;

float Bits(uint32_t v) { float f; std::memcpy(&f, &v, 4); return f; }

// D3DTSS_MIPMAPLODBIAS as the console's D3D gives it to the GPU: its signed 5.8 fixed-point field, -16 to just
// under 16. A NaN -- the Chapter 2 pond's ripple maps have FFFFFFFF -- comes out as 0x1000, -16 (xemu's
// register model, fed by the game's own D3D code, shows it): the hardware then always reads the top level,
// and the game never fills the others (they hold noise). Given to Vulkan as a NaN, AMD took 0 and Nvidia
// other levels: the banded water of #16.
float LodBias(uint32_t raw) {
    float b = Bits(raw);
    if (!(b == b)) return -16.0f;
    return std::clamp(std::floor(b * 256.0f), -4096.0f, 4095.0f) / 256.0f;
}

void Color(uint32_t argb, float* o) {
    o[0] = ((argb >> 16) & 255) / 255.0f;
    o[1] = ((argb >> 8) & 255) / 255.0f;
    o[2] = (argb & 255) / 255.0f;
    o[3] = (argb >> 24) / 255.0f;
}

uint32_t SimpleState(const State& s, uint32_t method) { return s.simple[method >> 2]; }

// D3D's enums to Vulkan's
VkBlendFactor Blend(uint32_t v, bool alpha) {
    switch (v) {
    case 0: return VK_BLEND_FACTOR_ZERO;
    case 1: return VK_BLEND_FACTOR_ONE;
    case 0x300: return alpha ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_SRC_COLOR;
    case 0x301: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case 0x302: return VK_BLEND_FACTOR_SRC_ALPHA;
    case 0x303: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 0x304: return VK_BLEND_FACTOR_DST_ALPHA;
    case 0x305: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case 0x306: return alpha ? VK_BLEND_FACTOR_DST_ALPHA : VK_BLEND_FACTOR_DST_COLOR;
    case 0x307: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case 0x308: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case 0x8001: return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case 0x8002: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case 0x8003: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case 0x8004: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    default: return VK_BLEND_FACTOR_ONE;
    }
}

VkBlendOp BlendOp(uint32_t v) {
    switch (v) {
    case 0x800A: return VK_BLEND_OP_SUBTRACT;
    case 0x800B: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case 0x8007: return VK_BLEND_OP_MIN;
    case 0x8008: return VK_BLEND_OP_MAX;
    default: return VK_BLEND_OP_ADD;
    }
}

// D3DCMP_NEVER (0x200) .. ALWAYS (0x207) in Vulkan's order, which is the same
VkCompareOp Cmp(uint32_t v) { return VkCompareOp(v >= 0x200 && v <= 0x207 ? v - 0x200 : 7); }

VkStencilOp StencilOp(uint32_t v) {
    switch (v) {
    case 0: return VK_STENCIL_OP_ZERO;
    case 0x1E01: return VK_STENCIL_OP_REPLACE;
    case 0x1E02: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case 0x1E03: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case 0x150A: return VK_STENCIL_OP_INVERT;
    case 0x8507: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case 0x8508: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default: return VK_STENCIL_OP_KEEP;
    }
}

VkSamplerAddressMode Address(uint32_t v) {
    switch (v) {
    case 2: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case 3: case 5: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case 4: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkFormat VertexFormat(uint8_t dxgi) {
    switch (dxgi) {
    case 2: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case 6: return VK_FORMAT_R32G32B32_SFLOAT;
    case 13: return VK_FORMAT_R16G16B16A16_SNORM;
    case 14: return VK_FORMAT_R16G16B16A16_SINT;
    case 16: return VK_FORMAT_R32G32_SFLOAT;
    case 28: return VK_FORMAT_R8G8B8A8_UNORM;
    case 37: return VK_FORMAT_R16G16_SNORM;
    case 38: return VK_FORMAT_R16G16_SINT;
    case 41: return VK_FORMAT_R32_SFLOAT;
    case 42: return VK_FORMAT_R32_UINT;
    case 49: return VK_FORMAT_R8G8_UNORM;
    case 58: return VK_FORMAT_R16_SNORM;
    case 59: return VK_FORMAT_R16_SINT;
    case 61: return VK_FORMAT_R8_UNORM;
    case 87: return VK_FORMAT_B8G8R8A8_UNORM;
    default: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

VkFormat TextureFormat(Host h) {
    switch (h) {
    case Host::BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case Host::BC2: return VK_FORMAT_BC2_UNORM_BLOCK;
    case Host::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
    case Host::R32F: return VK_FORMAT_R32_SFLOAT;
    case Host::R16Unorm: return VK_FORMAT_R16_UNORM;
    default: return VK_FORMAT_B8G8R8A8_UNORM;
    }
}

constexpr VkFormat kColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

// DXT1-5 blocks (w x h texels, rows of 4x4 blocks) to BGRA8, for a texture whose mipmaps are made here
void DecodeBC(Host h, const uint8_t* src, uint32_t w, uint32_t h_, std::vector<uint8_t>& out) {
    out.assign(size_t(w) * h_ * 4, 0);
    uint32_t bw = (w + 3) / 4, bh = (h_ + 3) / 4, bsize = h == Host::BC1 ? 8 : 16;
    for (uint32_t by = 0; by < bh; by++)
        for (uint32_t bx = 0; bx < bw; bx++) {
            const uint8_t* b = src + (size_t(by) * bw + bx) * bsize;
            const uint8_t* c = h == Host::BC1 ? b : b + 8;
            uint16_t c0 = uint16_t(c[0] | c[1] << 8), c1 = uint16_t(c[2] | c[3] << 8);
            uint8_t pal[4][4];   // B G R A
            auto rgb = [](uint16_t v, uint8_t* o) {
                o[2] = uint8_t((v >> 11) * 255 / 31);
                o[1] = uint8_t(((v >> 5) & 63) * 255 / 63);
                o[0] = uint8_t((v & 31) * 255 / 31);
                o[3] = 255;
            };
            rgb(c0, pal[0]);
            rgb(c1, pal[1]);
            bool four = h != Host::BC1 || c0 > c1;
            for (int k = 0; k < 3; k++) {
                pal[2][k] = uint8_t(four ? (2 * pal[0][k] + pal[1][k]) / 3 : (pal[0][k] + pal[1][k]) / 2);
                pal[3][k] = uint8_t(four ? (pal[0][k] + 2 * pal[1][k]) / 3 : 0);
            }
            pal[2][3] = 255;
            pal[3][3] = four ? 255 : 0;
            uint32_t idx = uint32_t(c[4] | c[5] << 8 | c[6] << 16 | uint32_t(c[7]) << 24);
            uint8_t alpha[16];
            if (h == Host::BC2) {
                for (int i = 0; i < 16; i++) alpha[i] = uint8_t(((b[i / 2] >> ((i & 1) * 4)) & 15) * 17);
            } else if (h == Host::BC3) {
                uint8_t a0 = b[0], a1 = b[1], ap[8] = {a0, a1};
                for (int i = 2; i < 8; i++)
                    ap[i] = a0 > a1 ? uint8_t(((8 - i) * a0 + (i - 1) * a1) / 7)
                            : i < 6 ? uint8_t(((6 - i) * a0 + (i - 1) * a1) / 5) : (i == 6 ? 0 : 255);
                uint64_t bits = 0;
                for (int i = 0; i < 6; i++) bits |= uint64_t(b[2 + i]) << (8 * i);
                for (int i = 0; i < 16; i++) alpha[i] = ap[(bits >> (3 * i)) & 7];
            }
            for (int i = 0; i < 16; i++) {
                uint32_t x = bx * 4 + uint32_t(i & 3), y = by * 4 + uint32_t(i >> 2);
                if (x >= w || y >= h_) continue;
                uint8_t* o = &out[(size_t(y) * w + x) * 4];
                std::memcpy(o, pal[(idx >> (2 * i)) & 3], 4);
                if (h != Host::BC1) o[3] = alpha[i];
            }
        }
}
constexpr VkDeviceSize kRingSize = 64u << 20;   // vertices, indices and constants of one frame
constexpr uint32_t kQuadVertices = 6;
extern "C" int ngb_pc_setting(int which);
extern "C" int nv2a_dirty_stamp(const void* host, size_t len, uint64_t* stamp);   // xboxrecomp nv2a_dirty.c
extern "C" float ngb_hud_shape(void);   // src/pc_menu.c: the menus' and HUD's narrowing
extern "C" int ngb_2d_stretched(void);
extern "C" [[noreturn]] void ngb_renderer_fatal(const char* detail);  // src/ingame_menu.c  // src/pc_menu.c: the game in its widescreen mode, its 2D as drawn
extern "C" int ngb_2d_screen_flags(void);   // src/pc_menu.c: ngb_2d_screen_frame's parts, for gpu::Peek
extern "C" int ngb_2d_screen_frame(int threed);   // src/pc_menu.c: a 2D screen (a menu, the title, a chapter
                                                // card), knowing the 3D this frame has drawn so far
// This frame's draws into display-sized targets, all and 3D (src/pc_menu.c: a frame of next to no 3D is a
// 2D screen, which the game draws in its widescreen mode)
extern "C" {
int ngb_frame_draws = 0, ngb_frame_3d_draws = 0;
}
int g_step_frame = -1;
// F7 (src/ingame_menu.c): record the next frames -- each a small picture, and every draw a line -- to
// ~/ngb-trace-<n>/, for a glitch that has to be caught in play
extern "C" { volatile int ngb_trace_request; }   // NGB_D3D_STEP's frame; also the next one after dump_dir/step.now appears

#include "glow_options.h"

// an image of ours: a render target or a texture
struct Image {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation mem = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t w = 0, h = 0, layers = 1, levels = 1, samples = 1;
    bool depth = false;
};

struct Target {
    Image img;
    Image ms;                  // MSAA: drawn into, resolved into img at the end of each pass
    uint32_t gw = 0, gh = 0;   // guest size
    uint32_t samples = 1;
    bool drawn = false;
    bool ms_stale = false;     // img holds the picture, ms not yet: filled from img before the next pass
    Image fb;                  // a copy of img for a draw that reads the target it draws into (BindTexture)
    uint32_t addr = 0;         // its guest memory (physical)
    uint64_t stamp = 0;        // the CPU's writes to that memory when last drawn into (StampTarget)
    bool stamped = false;
};

struct Tex {
    Image img;
    int gen = -1;              // made with generated mipmaps (1), or as given (0)
    uint64_t sig = 0;          // the guest bytes (and palette) it was made from
    uint32_t used = 0;         // frame last used
    TexInfo info;
    std::vector<uint8_t> check;   // NGB_D3D_BUMPCHECK: a bump map's top level as uploaded, to read back against
};

// PipeKey::has_ds: 1 a depth target; kDsDynamicBias the depth bias set at the draw (the map pieces' pull, below):
// a bit of an existing field, so the key -- and every pipeline the caches and the seed keep -- stays as it was
constexpr uint32_t kDsDynamicBias = 2;
struct PipeKey {
    uint64_t vs, ps;
    uint32_t regs, elements[16], strides[16];
    uint32_t topo, strip;
    uint32_t blend, src, dst, op, wmask;
    uint32_t zen, zwrite, zfunc, sten, sfunc, smask, swmask, sfail, szfail, spass;
    int32_t zbias;
    float zslope;
    uint32_t cull, ccw, cullfront, fill;
    uint32_t has_rt, has_ds, ds_format, samples;
    bool operator==(const PipeKey& o) const { return std::memcmp(this, &o, sizeof(*this)) == 0; }
};
struct PipeHash {
    size_t operator()(const PipeKey& k) const { return size_t(Hash(&k, sizeof(k))); }
};

// a texture's top level to dir: DDS for DXT, PPM for BGRA8 (alpha dropped); name_or_null: tex_<addr>_<w>x<h>
void DumpTexture(const TextureDesc& d, const FormatInfo& fi, const uint8_t* guest, const uint32_t* palette,
                 const char* dir, const char* name) {
    std::vector<uint8_t> blk;
    uint32_t pitch0 = 0;
    DecodeLevel(d, 0, 0, guest, palette, blk, pitch0);
    char path[512], base[128];
    if (!name) {
        std::snprintf(base, sizeof(base), "tex_%08X_%ux%u", d.data, d.width, d.height);
        name = base;
    }
    if (fi.host == Host::BC1 || fi.host == Host::BC3 || fi.host == Host::BC2) {
        std::snprintf(path, sizeof(path), "%s/%s.dds", dir, name);
        if (FILE* f = std::fopen(path, "wb")) {
            uint32_t h[32] = {};
            h[0] = 0x20534444;  // "DDS "
            h[1] = 124;
            h[2] = 0x1007;
            h[3] = d.height;
            h[4] = d.width;
            h[5] = uint32_t(blk.size());
            h[19] = 32;
            h[20] = 4;
            h[21] = fi.host == Host::BC1 ? 0x31545844 : fi.host == Host::BC2 ? 0x33545844 : 0x35545844;
            h[27] = 0x1000;
            std::fwrite(h, 4, 32, f);
            std::fwrite(blk.data(), 1, blk.size(), f);
            std::fclose(f);
        }
    } else if (fi.host == Host::BGRA8 && blk.size() >= size_t(d.width) * d.height * 4) {
        std::snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
        if (FILE* f = std::fopen(path, "wb")) {
            std::fprintf(f, "P6\n%u %u\n255\n", d.width, d.height);
            for (uint32_t i = 0; i < d.width * d.height; i++) {
                uint8_t rgb[3] = {blk[i * 4 + 2], blk[i * 4 + 1], blk[i * 4]};
                std::fwrite(rgb, 1, 3, f);
            }
            std::fclose(f);
        }
    }
}

class VulkanBackend final : public Backend {
public:
    ~VulkanBackend() override;
    bool Init(uint32_t width, uint32_t height) override;
    void Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color, float z,
               uint32_t stencil) override;
    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) override;
    void Copy(const Surface& src, const Surface& dst, uint32_t count, const uint8_t* src_rects,
              const uint8_t* dst_points) override;
    void Present(const Surface& front) override;
    void Sync(uint32_t, uint32_t) override {}
    void SyncLate(uint32_t) override {}
    void Flush() override { Submit(); }
    void SetGamma(const uint8_t* ramp) override;
    void VisibilityBegin() override { vis_on_ = true; vis_test_++; }
    void VisibilityEnd(uint32_t report_va) override;
    void CapturePeek(const State& s, Peek& p) override;
    void CaptureQuick(QuickPeek& q) override;
    void UsePeek(const Peek& p) override { pk_ = p; pk_given_ = true; }
    void UseQuick(const QuickPeek& q) override { qk_ = q; }

private:
    // the game's side of the call being drawn (gpu::Peek) and of the frame (QuickPeek): taken here at the call, or
    // given by a render thread's capture (pk_given_) -- the drawing reads these, never the game itself
    Peek pk_;
    QuickPeek qk_;
    bool pk_given_ = false;
    uint32_t towers_lo_ = 0, towers_hi_ = 0;   // the towers' chunk's data, up to the next loaded chunk's
    uint32_t towers_frame_ = ~0u, peek_frames_ = 0;
    // ngb_2d_screen_frame as of the call: a 2D screen, knowing the 3D drawn so far this frame
    bool ScreenFrame(uint32_t threed) const { return (pk_.screen & 1) || ((pk_.screen & 2) && threed < 30); }
    // device: the NV2A renderer's, shared, when NGB_RENDERER=d3d (its presenter then shows our
    // frames in the game window); else one of our own, drawing unseen (mirror mode)
    bool CreateInstance();
    bool CreateDevice();
    bool AdoptDevice(const nv2a_vk_shared& sh);
    bool FinishDevice();
    bool shared_ = false;
    void QueueSubmit(const VkSubmitInfo& si, VkFence fence);
    struct ContinuousRecord { uint32_t data=0,frame=~0u; };
    ContinuousRecord continuous_record_[3];
    Image continuous_glow_[3];
    uint32_t continuous_hits_[3]{};
    void InvalidateContinuousGlow(uint32_t data);
    int ContinuousGlowStage(const State& s,const Assembled& a,uint32_t count);
    bool ContinuousGlowPass(Image& input,Image& output,uint32_t w,uint32_t h,int stage,const float* threshold);
    void ApplyContinuousGlow(const State& s,const Assembled& a,uint32_t count,Target& rt,const DrawConstants& c);
    Image glow_soft_[2];
    VkSampler glow_soft_sampler_=VK_NULL_HANDLE;
    bool SoftenGlow(Image& source);
    std::set<uint32_t> glow_targets_;
    void ClassifyGlowTarget(const State& s,const Assembled& a,uint32_t count);

    Image display_[3];   // copies of the front buffer for the presenter, in turn
    Image quick_layer_;
    Image quick_background_, quick_blur_, quick_full_;   // the quick menu's paused background: small, its blur's
                                                          // other half, and the frame blitted for it
    VkSampler quick_sampler_ = VK_NULL_HANDLE;
    void QuickMenu(Image& dst);
    void QuickMenuBackground(Image& dst);
    // the title's gamma ramp (D3DDevice_SetGammaRamp: its Screen Adjustment -- gamma, brightness,
    // contrast), applied as the front buffer is copied for the presenter; identity: a plain copy
    uint8_t gamma_[768] = {};
    bool gamma_on_ = false, gamma_dirty_ = false;
    Image gamma_lut_;    // 256 x 1, each channel its ramp
    // src into dst through the gamma ramp (gamma_on_), with black beyond the middle's edges (bars: the
    // picture's part at each side, 0 none)
    void GammaCopy(Image& src, Image& dst, float bars = 0, float bars_y = 0);
    // The bars beside a 2D screen filled with its own colours, still: the picture's edges (16 bands a side,
    // rows left, right, top, bottom), blended in over the screen's first 2 s, then kept as they are --
    // a menu's animation does not move them (copied each frame, the sides flickered with it)
    Image edge_;
    VkSampler edge_sampler_ = VK_NULL_HANDLE;
    uint32_t edge_frames_ = 0;
    bool edge_use_ = false;   // (else the bars black: null2d_, cleared)
    void EdgeSample(Image& src, float bars, float bars_y);
    int display_next_ = 0;
    // the quality settings (nv2a_quality_get), read once a frame: display-sized targets render
    // at scale_ times their size, small depth-only ones (shadow maps) at shadow_ times
    int scale_ = 1, shadow_ = 1, aniso_level_ = 0, msaa_ = 1;
    bool anisotropy_ = false;
    bool wide_lines_ = false;   // lines wider than a pixel (the sparks of a hit are 2.7 to 3, #51)
    float point_max_ = 1.0f;
    float view_yaw_ = 0, view_pitch_ = 0;   // NGB_D3D_VIEWLOG
    bool view_seen_ = false;
    float lod_bias_max_ = 0.0f;   // maxSamplerLodBias    // the largest point (largePoints): the particles are point sprites (#51)
    float line_max_ = 1.0f;
    VkSampleCountFlags ms_counts_ = VK_SAMPLE_COUNT_1_BIT;   // the device's for colour and depth together
    void ReadQuality();
    float hud_cover_ = 0;      // this frame's 2D picture area, in screens
    bool hud_menu_ = false;    // the frame before was a full-screen menu (most of it 2D pictures)
    float gauge_x_[4][2] = {}, gauge_prev_[4][2] = {};   // tall HUD gauges' columns (this frame, the one before)
    // The HUD in play on a screen not 16:9 (HudPlace): this frame's pieces, and the elements -- pieces near
    // each other, together -- of the frame before, each with the side it keeps to
    struct HudBox { float x0, x1, y0, y1; uint32_t at; };   // (at: the draw's number in its frame)
    struct HudElem { float x0, x1, y0, y1, ax, ay; uint32_t seen; bool gauge; };
    std::vector<HudBox> hud_boxes_;
    std::vector<HudBox> hud_gauge_boxes_;   // the frame before's pieces of tall side gauges (hud_gauge_piece)
    std::vector<HudElem> hud_elems_, hud_memory_;   // the frame before's; and those of the last half minute
    // the frame's last 3D draw into the back buffer (its number): the HUD is drawn after it, what is drawn in
    // screen space before it is the 3D's (mist, spray) -- not grouped as the HUD (a draw cannot know it as it
    // is made: how many 3D draws a frame has changes from frame to frame)
    uint32_t last3d_now_ = 0;
    float hud_box_cover_ = 0;
    // a menu over the paused game (the save statue's): the paused picture drawn this frame, and how much of
    // the screen the 2D drawn over it covers -- little, and the game shows across the screen instead of bars
    bool paused_now_ = false;
    bool covered_now_ = false;   // this frame drew an opaque colour over the whole screen (so far)
    uint32_t pic_tex_ = 0;   // this frame's opaque picture across the screen so far: its texture, its right end
    float pic_x1_ = -1;
    float paused_cover_ = 0;   // how much of the screen the frame before's HUD pieces covered (the trace's)
    // a screen of panels over the game (a chapter clear's): an element large and about the middle, for 4 frames
    // running; over once there has been none for 8
    bool hud_panel_ = false;
    int hud_panel_count_ = 0;
    HudBox hud_panel_box_{}, hud_panel_seen_{};   // the panel's box (the union of the frames it was seen in)
    void HudPlace(float x0, float x1, float y0, float y1, float w, float h, float& ax, float& ay);
    void HudGroup(float w, float h);
    int gauges_now_ = 0;
    uint32_t offscreen3d_ = 0;   // this frame's 3D draws into smaller targets (shadows, reflections, effects)
    uint32_t threed_ = 0, threed_prev_ = 0;   // 3D draws into the back buffer, this frame and the one before:
                                              // a screen with them is play (an underwater overlay is no menu)
    VolkDeviceTable d_{};
    struct SigMemo { uint64_t frame = 0, sig = 0, stamp = 0; bool stamped = false; };
    std::unordered_map<uint64_t, SigMemo> sig_memo_;   // texture bytes' signature: this frame's, and its pages' writes

    // the title's visibility tests (the sun's lens flare, #35): each draw in one counted by an occlusion query,
    // per frame slot; when the slot's frame is done, the counts summed per test, in console pixels, and
    // written to each ended test's report slot as the GPU would (xemu's renderer, not drawing, would say 0)
    static constexpr uint32_t kVisQueries = 512;
    struct VisQuery { uint32_t test; float div; };
    VkQueryPool vis_pool_[2] = {};
    std::vector<VisQuery> vis_q_[2];
    std::vector<std::pair<uint32_t, uint32_t>> vis_end_[2];   // test, report slot
    std::unordered_map<uint32_t, double> vis_sum_;
    uint32_t vis_test_ = 0;
    bool vis_on_ = false;
    void VisResults(int slot);
    VkInstance inst_ = VK_NULL_HANDLE;
    VkPhysicalDevice phys_ = VK_NULL_HANDLE;
    VkDevice dev_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t qfam_ = 0;
    VmaAllocator vma_ = nullptr;
    VkFormat ds_format_ = VK_FORMAT_D24_UNORM_S8_UINT;

    // the frame's commands
    VkCommandPool pool_ = VK_NULL_HANDLE;
    // two frames in flight: the game records the next while the GPU draws the last; cmd_, fence_ and
    // the ring's half are the recording slot's
    VkCommandBuffer cmd_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    VkCommandBuffer cmds_[2] = {};
    VkFence fences_[2] = {};
    bool submitted_[2] = {};
    int slot_ = 0;
    void EndFrame();
    // images no longer used, destroyed once no frame in flight can use them
    struct Retired { Image img; VkImageView view; uint32_t frame; };
    std::vector<Retired> graveyard_;
    const bool bumpcheck_ = std::getenv("NGB_D3D_BUMPCHECK") != nullptr;
    uint64_t bump_key_ = 0;           // textures_ key of the bump map last bound (NGB_D3D_BUMPCHECK)
    void BumpCheck(Tex& tx, const char* when);
    std::set<uint32_t> told_size_;    // textures logged as reading a target of another size (#16), or their own (| 1)
    void Retire(Image& img, VkImageView extra = VK_NULL_HANDLE);
    size_t EvictTextures(uint32_t idle_frames);   // textures unused that many frames: retired (#81)
    void QuadSnap(const State& s, const Assembled& a, uint32_t count, bool quad, uint32_t samples, DrawConstants& c);
    bool recording_ = false;
    uint32_t id_count_ = 0, id_file_frame_ = ~0u;   // NGB_D3D_IDVIEW=2
    FILE* id_file_ = nullptr;
    uint32_t section_pulled_ = 0;   // the drawbridge towers' draws pulled this frame (NGB_D3D_SECTION_LOG)
    void Begin();
    void Submit();
    uint32_t frame_ = 0;

    // the upload ring
    VkBuffer ring_ = VK_NULL_HANDLE;
    VmaAllocation ring_mem_ = nullptr;
    uint8_t* ring_cpu_ = nullptr;
    VkDeviceSize ring_pos_ = 0;
    VkDeviceSize ringAlign_ = 256;
    VkDeviceSize Upload(const void* data, VkDeviceSize bytes, VkDeviceSize align);

    // images
    bool CreateImage(Image& img, VkFormat fmt, uint32_t w, uint32_t h, uint32_t layers, uint32_t levels, bool depth,
                     bool cube, VkImageUsageFlags usage, uint32_t samples = 1, bool mutable_format = false);
    void DestroyImage(Image& img);
    void Barrier(Image& img, VkImageLayout to);

    // targets
    std::unordered_map<uint64_t, Target> targets_;
    Target* EnsureTarget(const Surface& s, bool depth);
    Target* MakeTarget(const Surface& s, bool depth, uint64_t key, uint32_t w, uint32_t h, uint32_t iw, uint32_t ih,
                       uint32_t samples);
    Target* bound_rt_ = nullptr;
    Target* draw_rt_ = nullptr;      // the colour target of the draw whose textures are being bound
    Target* bound_ds_ = nullptr;
    bool rendering_ = false;
    void BeginRendering(Target* rt, Target* ds);
    bool TargetStamp(const Target& t, uint64_t& stamp);
    void StampTarget(Target& t) { t.stamped = TargetStamp(t, t.stamp); }
    void EndRendering();
    void FillMs(Target& t);    // MSAA image from the plain one (a picture kept across a setting change)
    VkSampler fill_sampler_ = VK_NULL_HANDLE;

    // textures
    std::unordered_map<uint64_t, Tex> textures_;
    VkImageView BindTexture(const State& s, int stage, TexInfo& info, bool& from_target);
    Image null2d_, nullcube_, null3d_;
    std::unordered_map<uint64_t, VkSampler> samplers_;
    VkSampler Sampler(const State& s, int stage, const TexInfo& info);

    // shaders and pipelines
    VkDescriptorSetLayout set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipe_layout_ = VK_NULL_HANDLE;
    std::unordered_map<uint64_t, VkShaderModule> modules_;
    std::unordered_map<PipeKey, VkPipeline, PipeHash> pipelines_;
    VkShaderModule Module(uint64_t key, const std::function<std::string()>& src, bool vertex);
    VkPipeline Pipeline(const PipeKey& k);

    Assembled asm_;
    // the shader cache (ShaderCache below): how each module was made, its SPIR-V, the pipelines
    std::recursive_mutex cache_mu_;   // modules_ and pipelines_ (the prewarm thread fills them too)
    std::mutex compile_mu_;           // glslang, one at a time
    std::string cache_dir_;
    FILE* recipes_ = nullptr;         // cache_dir_/recipes.bin: modules' inputs, pipeline keys (appended)
    FILE* spirv_ = nullptr;           // cache_dir_/spirv.bin: SPIR-V by the hash of its HLSL (appended)
    FILE* seed_out_ = nullptr;        // NGB_D3D_SEED_RECORD: the same records, for a shipped seed
    std::unordered_map<uint64_t, std::vector<uint32_t>> spirv_by_src_;
    VkPipelineCache pcache_ = VK_NULL_HANDLE;
    uint32_t pcache_saved_ = 0;
    std::atomic<uint32_t> pipes_since_save_{0};
    std::thread saver_;                      // the periodic save of pcache_, off the frame (CacheSavePipelinesSoon)
    std::atomic<bool> saving_{false};
    // shaders and pipelines a draw needs and nothing has made yet: made at once while the frame has spent less
    // than compile_budget_us_ making them, then on a thread of their own while the draw is left out (a frame or
    // two) -- a short wait instead of geometry missing (#94), and no long hitch when an area brings dozens.
    // NGB_D3D_COMPILE_BUDGET_MS sets the budget (4; 0: always on the thread), NGB_D3D_SYNC_COMPILE=1 makes all at once.
    struct Job { bool pipeline; uint64_t key; std::string hlsl; bool vertex; PipeKey pk; };
    std::thread compiler_;
    std::mutex job_mu_;
    std::condition_variable job_cv_;
    std::deque<Job> jobs_;
    std::unordered_map<uint64_t, bool> pending_mods_;
    std::unordered_map<PipeKey, bool, PipeHash> pending_pipes_;
    std::unordered_map<PipeKey, bool, PipeHash> known_pipes_;   // the seed's and earlier sessions' (cache_mu_)
    bool async_ = true;      // the setting
    bool async_now_ = false; // for the draw being made (the backend's own passes make theirs at once)
    uint64_t compile_budget_us_ = 4000;
    bool Defer() const { return async_now_ && compile_us_ >= compile_budget_us_; }   // this one to the thread
    void Compiler();
    std::atomic<uint32_t> async_made_{0}, skipped_{0};
    std::thread prewarm_;
    std::atomic<bool> stop_{false};
    std::atomic<uint32_t> prewarm_done_{0};
    void CacheOpen();
    void CacheRecord(char kind, uint64_t key, const void* data, size_t n, FILE* only = nullptr);
    void CacheSavePipelines();
    void CacheSavePipelinesSoon();
    void CacheWritePipelines();
    void Prewarm();
    void WarmSamples(uint32_t samples);
    uint32_t warmed_samples_ = 0;
    std::atomic<uint32_t> warm_want_{0};   // the sample count being warmed: another stops the one under way
    bool Compile(const std::string& hlsl, bool vertex, std::vector<uint32_t>& spirv);
    bool SpirvKnown(const std::string& hlsl, bool vertex);
    VkShaderModule MakeModule(const std::vector<uint32_t>& spirv);
    VkPipeline MakePipeline(const PipeKey& k, VkShaderModule vs, VkShaderModule ps);
    std::string CurrentVertexProgramRecipe(const std::vector<uint32_t>& prog, uint16_t regs, uint64_t decode,
                                           uint32_t quad, std::vector<uint8_t>& out);
    uint64_t compile_us_ = 0;      // this frame's shader and pipeline making
    uint32_t compiles_ = 0, pipes_made_ = 0;

    // NGB_D3D_STATS: per frame, what the draws came to
    struct Stats { uint32_t draws, noasm, notarget, nopipe, drawn, clears, copies; } st_{};
    std::unordered_map<uint32_t, uint32_t> rt_draws_;
    bool stats_ = false;

    // dumps
    std::string dump_dir_;
    FILE* trace_ = nullptr;   // F7's recording: draws.txt
    std::string trace_dir_;
    int trace_left_ = 0;
    uint32_t trace_pic_until_ = 0;   // NGB_TRACE_NOPIC=copy: pictures only around a copy into a 512x512 target
    Image trace_img_;         // the front buffer, small, for its picture
    void TraceFrame(Target& t);
    uint32_t dump_from_ = 0, dump_count_ = 0, presents_ = 0;
    void Dump(Target& t, const char* name = nullptr);
};

#define VKCHECK(x)                                                                  \
    do {                                                                            \
        VkResult r_ = (x);                                                          \
        if (r_ != VK_SUCCESS) {                                                     \
            std::fprintf(stderr, "[D3D] vulkan: %s failed (%d)\n", #x, int(r_));    \
            return false;                                                           \
        }                                                                           \
    } while (0)

bool VulkanBackend::CreateInstance() {
    if (volkGetInstanceVersion() == 0 && volkInitialize() != VK_SUCCESS) return false;
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Ninja Gaiden Black (Direct3D renderer)";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
#ifdef __APPLE__
    const char *extensions[] = {VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = extensions;
    ci.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    VKCHECK(vkCreateInstance(&ci, nullptr, &inst_));
    // the instance functions: shared with the NV2A renderer's when it has loaded them (loader
    // trampolines, good for either instance), else ours
    if (!volkGetLoadedInstance()) volkLoadInstanceOnly(inst_);
    return true;
}

bool VulkanBackend::CreateDevice() {
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst_, &n, nullptr);
    std::vector<VkPhysicalDevice> devs(n);
    vkEnumeratePhysicalDevices(inst_, &n, devs.data());
    for (VkPhysicalDevice p : devs) {
        VkPhysicalDeviceProperties pr;
        vkGetPhysicalDeviceProperties(p, &pr);
        if (pr.apiVersion < VK_API_VERSION_1_3) continue;
        if (!phys_ || pr.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) phys_ = p;
    }
    if (!phys_) {
        std::fprintf(stderr, "[D3D] vulkan: no Vulkan 1.3 device\n");
        return false;
    }
    uint32_t qn = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_, &qn, nullptr);
    std::vector<VkQueueFamilyProperties> qf(qn);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_, &qn, qf.data());
    for (uint32_t i = 0; i < qn; i++)
        if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            qfam_ = i;
            break;
        }
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = qfam_;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    f13.dynamicRendering = VK_TRUE;
    f13.synchronization2 = VK_TRUE;
    VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    f2.pNext = &f13;
    f2.features.samplerAnisotropy = VK_TRUE;
    f2.features.fillModeNonSolid = VK_TRUE;
    f2.features.textureCompressionBC = VK_TRUE;
    {
        VkPhysicalDeviceFeatures have;
        vkGetPhysicalDeviceFeatures(phys_, &have);
        f2.features.wideLines = have.wideLines;
        f2.features.largePoints = have.largePoints;
    }
    const char* exts[] = {VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
#ifdef __APPLE__
                          "VK_KHR_portability_subset",
#endif
    };
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.pNext = &f2;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = sizeof(exts) / sizeof(exts[0]);
    dci.ppEnabledExtensionNames = exts;
    VKCHECK(vkCreateDevice(phys_, &dci, nullptr, &dev_));
    volkLoadDeviceTable(&d_, dev_);
    d_.vkGetDeviceQueue(dev_, qfam_, 0, &queue_);
    return FinishDevice();
}

bool VulkanBackend::AdoptDevice(const nv2a_vk_shared& sh) {
    if (volkGetInstanceVersion() == 0 && volkInitialize() != VK_SUCCESS) return false;
    inst_ = sh.instance;
    phys_ = sh.physical_device;
    dev_ = sh.device;
    qfam_ = sh.queue_family;
    queue_ = sh.queue;
    if (!volkGetLoadedInstance()) volkLoadInstanceOnly(inst_);
    volkLoadDeviceTable(&d_, dev_);
    shared_ = true;
    return FinishDevice();
}

bool VulkanBackend::FinishDevice() {
    VmaAllocatorCreateInfo ai{};
    ai.physicalDevice = phys_;
    ai.device = dev_;
    ai.instance = inst_;
    ai.vulkanApiVersion = VK_API_VERSION_1_3;
    VmaVulkanFunctions fns{};
    VKCHECK(vmaImportVulkanFunctionsFromVolk(&ai, &fns));
    ai.pVulkanFunctions = &fns;
    VKCHECK(vmaCreateAllocator(&ai, &vma_));

    // 24-bit depth, as the console has, where the device has it (Nvidia); else float (AMD). 0.9.17 gave
    // Nvidia float depth too, for the shadow blotches of #29: they stayed, and second passes over a drawn
    // surface came out flickering or missing on Nvidia (#51, #53). NGB_D3D_DEPTH32=1: float, to compare.
    VkFormatProperties fp;
    bool d32 = std::getenv("NGB_D3D_DEPTH32") != nullptr;
    vkGetPhysicalDeviceFormatProperties(phys_, VK_FORMAT_D24_UNORM_S8_UINT, &fp);
    ds_format_ = !d32 && (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                     ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT;
    std::fprintf(stderr, "[D3D] depth: %s\n", ds_format_ == VK_FORMAT_D32_SFLOAT_S8_UINT ? "32-bit float" : "24-bit");
    VkPhysicalDeviceProperties pr;
    vkGetPhysicalDeviceProperties(phys_, &pr);
    ringAlign_ = std::max<VkDeviceSize>(pr.limits.minUniformBufferOffsetAlignment, 16);
    VkPhysicalDeviceFeatures feat;
    vkGetPhysicalDeviceFeatures(phys_, &feat);   // both devices enable it when it is there
    anisotropy_ = feat.samplerAnisotropy == VK_TRUE;
    wide_lines_ = feat.wideLines == VK_TRUE;
    line_max_ = pr.limits.lineWidthRange[1];
    point_max_ = feat.largePoints == VK_TRUE ? pr.limits.pointSizeRange[1] : 1.0f;
    lod_bias_max_ = pr.limits.maxSamplerLodBias;
    ms_counts_ = pr.limits.framebufferColorSampleCounts & pr.limits.framebufferDepthSampleCounts &
                 pr.limits.framebufferStencilSampleCounts;
    ReadQuality();
    std::fprintf(stderr, "[D3D] vulkan: %s%s\n", pr.deviceName, shared_ ? " (the NV2A renderer's device)" : "");
    return true;
}

void VulkanBackend::ReadQuality() {
    int msaa = 1;
    nv2a_quality_get(&scale_, &aniso_level_, &shadow_, &msaa);
    while (msaa > 1 && !(ms_counts_ & uint32_t(msaa))) msaa /= 2;   // the most the device has
    msaa_ = msaa;
    // a new anti-aliasing setting: its pipelines made in the background once the startup ones are
    if (prewarm_done_ && warmed_samples_ != uint32_t(msaa)) {
        if (!warmed_samples_) {
            warmed_samples_ = uint32_t(msaa);   // the startup warm-up did this one
        } else {
            warmed_samples_ = uint32_t(msaa);
            warm_want_ = uint32_t(msaa);
            if (prewarm_.joinable()) prewarm_.join();
            prewarm_ = std::thread([this, msaa] { WarmSamples(uint32_t(msaa)); });
        }
    }
    static const char* scale_env = std::getenv("NGB_D3D_SCALE");
    static const char* flip_env = std::getenv("NGB_D3D_QUALITY_FLIP");
    if (const char* e = scale_env) scale_ = std::clamp(std::atoi(e), 1, 10);   // debugging
    if (const char* e = flip_env) {   // testing: MSAA and scale changing every n s
        static uint32_t start = presents_;
        uint32_t n = uint32_t(std::max(1, std::atoi(e)) * 60), k = (presents_ - start) / n;
        if (k & 1) {
            msaa_ = (ms_counts_ & 4) ? 4 : 1;
            scale_ = 3;
        }
    }
}

void VulkanBackend::QueueSubmit(const VkSubmitInfo& si, VkFence fence) {
    if (shared_) nv2a_vk_queue_lock();
    d_.vkQueueSubmit(queue_, 1, &si, fence);
    if (shared_) nv2a_vk_queue_unlock();
}

bool VulkanBackend::Init(uint32_t, uint32_t) {
    const char* mode = std::getenv("NGB_RENDERER");
    if (mode && !std::strcmp(mode, "d3d")) {
        nv2a_vk_shared sh{};
        int st = nv2a_vk_share(&sh);
        if (st == 0) {
            g_init_retry = true;   // the NV2A renderer is still starting
            return false;
        }
        if (st == 1 && sh.features13) {
            if (!AdoptDevice(sh)) return false;
            // the NV2A renderer keeps executing the pushbuffer (the title's fences, flips and
            // vertical blanks come from it) but no longer draws; NGB_D3D_KEEP_NV2A=1 keeps it
            // drawing (unseen) for comparisons
            if (!std::getenv("NGB_D3D_KEEP_NV2A")) nv2a_vk_set_draws_off(true);
        } else {
            ngb_renderer_fatal(st == 1 ? "The GPU's Vulkan driver has no Vulkan 1.3 dynamic rendering or push descriptors."
                                       : "No Vulkan device could be used.");
        }
    }
    if (!shared_ && (!CreateInstance() || !CreateDevice())) return false;

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = qfam_;
    VKCHECK(d_.vkCreateCommandPool(dev_, &pci, nullptr, &pool_));
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = pool_;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 2;
    VKCHECK(d_.vkAllocateCommandBuffers(dev_, &cai, cmds_));
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VKCHECK(d_.vkCreateFence(dev_, &fci, nullptr, &fences_[0]));
    VKCHECK(d_.vkCreateFence(dev_, &fci, nullptr, &fences_[1]));
    cmd_ = cmds_[0];
    fence_ = fences_[0];

    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = kRingSize;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo info;
    VKCHECK(vmaCreateBuffer(vma_, &bci, &aci, &ring_, &ring_mem_, &info));
    ring_cpu_ = static_cast<uint8_t*>(info.pMappedData);

    // set 0: VC, DC, four textures, four samplers; pushed per draw
    VkDescriptorSetLayoutBinding b[10] = {};
    for (int i = 0; i < 10; i++) {
        b[i].binding = uint32_t(i);
        b[i].descriptorCount = 1;
        b[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        b[i].descriptorType = i < 2 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                              : i < 6 ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    lci.bindingCount = 10;
    lci.pBindings = b;
    VKCHECK(d_.vkCreateDescriptorSetLayout(dev_, &lci, nullptr, &set_layout_));
    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &set_layout_;
    VKCHECK(d_.vkCreatePipelineLayout(dev_, &plci, nullptr, &pipe_layout_));

    // stand-ins for stages without a texture, of each kind
    VkImageUsageFlags u = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (!CreateImage(null2d_, kColorFormat, 1, 1, 1, 1, false, false, u) ||
        !CreateImage(nullcube_, kColorFormat, 1, 1, 6, 1, false, true, u) ||
        !CreateImage(null3d_, kColorFormat, 1, 1, 1, 1, false, false, u))
        return false;

    stats_ = std::getenv("NGB_D3D_STATS") != nullptr;
    if (const char* e = std::getenv("NGB_D3D_DUMP")) {
        dump_dir_ = e;
        dump_from_ = std::getenv("NGB_D3D_DUMP_FROM") ? uint32_t(std::atoi(std::getenv("NGB_D3D_DUMP_FROM"))) : 0;
        dump_count_ = std::getenv("NGB_D3D_DUMP_COUNT") ? uint32_t(std::atoi(std::getenv("NGB_D3D_DUMP_COUNT"))) : 1u << 30;
    }
    VkQueryPoolCreateInfo qci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qci.queryType = VK_QUERY_TYPE_OCCLUSION;
    qci.queryCount = kVisQueries;
    for (auto& p : vis_pool_)
        if (d_.vkCreateQueryPool(dev_, &qci, nullptr, &p) != VK_SUCCESS) p = VK_NULL_HANDLE;
    Begin();
    Barrier(null2d_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(nullcube_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(null3d_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    CacheOpen();
    return true;
}

VulkanBackend::~VulkanBackend() {
    stop_ = true;
    if (prewarm_.joinable()) prewarm_.join();
    {
        std::lock_guard<std::mutex> g(job_mu_);
        job_cv_.notify_all();
    }
    if (compiler_.joinable()) compiler_.join();
    if (saver_.joinable()) saver_.join();
    CacheSavePipelines();
    if (shared_) {
        nv2a_vk_set_draws_off(false);
        nv2a_present_set_external(VK_NULL_HANDLE, 0, 0);
        nv2a_vk_queue_lock();
        d_.vkDeviceWaitIdle(dev_);
        nv2a_vk_queue_unlock();
    } else if (dev_) {
        d_.vkDeviceWaitIdle(dev_);
    }
    if (dev_) {
        for (auto& image:continuous_glow_) DestroyImage(image);
        for (auto& image:glow_soft_) DestroyImage(image);
        DestroyImage(quick_background_);
        DestroyImage(quick_blur_);
        DestroyImage(quick_full_);
        if (glow_soft_sampler_) d_.vkDestroySampler(dev_,glow_soft_sampler_,nullptr);
    }
}

void VulkanBackend::Begin() {
    if (recording_) return;
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    d_.vkBeginCommandBuffer(cmd_, &bi);
    recording_ = true;
    if (vis_pool_[slot_]) d_.vkCmdResetQueryPool(cmd_, vis_pool_[slot_], 0, kVisQueries);
}

// Everything recorded so far, done: the slot's commands submitted and waited for, and the other
// slot's frame too (for a caller about to destroy or read back what they used).
void VulkanBackend::Submit() {
    if (!recording_) return;
    EndRendering();
    d_.vkEndCommandBuffer(cmd_);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd_;
    d_.vkResetFences(dev_, 1, &fence_);
    QueueSubmit(si, fence_);
    d_.vkWaitForFences(dev_, 1, &fence_, VK_TRUE, UINT64_MAX);
    if (submitted_[slot_ ^ 1]) {
        d_.vkWaitForFences(dev_, 1, &fences_[slot_ ^ 1], VK_TRUE, UINT64_MAX);
        submitted_[slot_ ^ 1] = false;
        VisResults(slot_ ^ 1);
    }
    submitted_[slot_] = false;
    VisResults(slot_);
    d_.vkResetCommandBuffer(cmd_, 0);
    recording_ = false;
    ring_pos_ = 0;
    Begin();
}

// The frame's commands to the GPU without waiting: the next frame records in the other slot, once
// the frame before this one (which used it) is done.
void VulkanBackend::EndFrame() {
    if (!recording_) return;
    EndRendering();
    d_.vkEndCommandBuffer(cmd_);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd_;
    d_.vkResetFences(dev_, 1, &fence_);
    QueueSubmit(si, fence_);
    submitted_[slot_] = true;
    recording_ = false;
    slot_ ^= 1;
    cmd_ = cmds_[slot_];
    fence_ = fences_[slot_];
    if (submitted_[slot_]) {
        d_.vkWaitForFences(dev_, 1, &fence_, VK_TRUE, UINT64_MAX);
        submitted_[slot_] = false;
        VisResults(slot_);
    }
    d_.vkResetCommandBuffer(cmd_, 0);
    ring_pos_ = 0;
    // what the finished frames last used
    for (size_t i = 0; i < graveyard_.size();) {
        if (graveyard_[i].frame + 2 <= frame_ + 1) {
            if (graveyard_[i].view) d_.vkDestroyImageView(dev_, graveyard_[i].view, nullptr);
            DestroyImage(graveyard_[i].img);
            graveyard_[i] = graveyard_.back();
            graveyard_.pop_back();
        } else {
            i++;
        }
    }
    if (frame_ % 60 == 0) {   // once a second: textures unused for 30 s go (EvictTextures)
        size_t gone = EvictTextures(1800);
        static const bool stats = std::getenv("NGB_D3D_TEXSTATS") != nullptr;
        if (stats) {
            uint64_t bytes = 0;
            for (const auto& t : textures_)
                bytes += uint64_t(t.second.img.w) * t.second.img.h * 4 * (t.second.gen == 1 ? 4 : 3) / 3;
            uint64_t tbytes = 0;   // the render targets: kept by guest address (EnsureTarget)
            for (const auto& t : targets_)
                tbytes += uint64_t(t.second.img.w) * t.second.img.h * 4 * std::max<uint32_t>(t.second.samples, 1);
            std::fprintf(stderr, "[TEXSTATS] frame %u: %zu textures, ~%llu MB, %zu evicted; %zu targets, ~%llu MB\n", frame_,
                         textures_.size(), (unsigned long long)(bytes >> 20), gone, targets_.size(),
                         (unsigned long long)(tbytes >> 20));
            VmaBudget bud[VK_MAX_MEMORY_HEAPS];
            vmaGetHeapBudgets(vma_, bud);
            VmaTotalStatistics ts;
            vmaCalculateStatistics(vma_, &ts);
            std::fprintf(stderr, "[TEXSTATS] heap0 usage %llu MB budget %llu MB; vma blocks %u allocs %u, %llu MB in blocks %llu MB used\n",
                         (unsigned long long)(bud[0].usage >> 20), (unsigned long long)(bud[0].budget >> 20),
                         ts.total.statistics.blockCount, ts.total.statistics.allocationCount,
                         (unsigned long long)(ts.total.statistics.blockBytes >> 20),
                         (unsigned long long)(ts.total.statistics.allocationBytes >> 20));
        }
    }
    Begin();
}

// Textures not used for a while give their memory back (#81). Each texture the game puts at a new address made
// an image that stayed for the rest of the session, so a long one -- the game streaming area after area into
// different memory -- filled the card's memory until an image could no longer be made and that texture was drawn
// black, which nothing but a restart cleared (the game's soft reset keeps the renderer). Retired, not destroyed:
// a frame still in flight may read it. Returns how many went.
size_t VulkanBackend::EvictTextures(uint32_t idle_frames) {
    static const bool off = std::getenv("NGB_D3D_NO_EVICT") != nullptr;   // NGB_D3D_NO_EVICT=1: kept, as before 0.9.29
    if (off) return 0;
    size_t n = 0;
    for (auto it = textures_.begin(); it != textures_.end();) {
        if (it->second.img.image && frame_ - it->second.used > idle_frames) {
            Retire(it->second.img);
            it = textures_.erase(it);
            n++;
        } else {
            ++it;
        }
    }
    return n;
}

// The Tairon drawbridge's towers (NGB_D3D_SECTION_PULL=n depth steps, default 2, and NGB_D3D_SECTION_SLOPE=f console
// pixels' worth of depth slope, default 0.5; NGB_D3D_SECTION_PULL=0 turns it off). Ninja Gaiden Black's levels load in
// chunks (the game's table at 0x9504C0, 0x14 bytes a slot: +0 the chunk's data, +4 1 while loaded; a map piece object
// -- 0x54 bytes, drawn through the render list walk -- names its chunk at +8 as the slot's address + 4). By the
// drawbridge the towers' section lays its rubble faces over the town section's block walls in the same plane, cut into
// different triangles; their depths tie within a few steps either way and the two textures flicker -- in either
// renderer, as xemu's model of the console's depth gives the same ties -- where the Xbox shows the rubble steadily.
// The towers' pieces are pulled nearer here, and only theirs: a rule for every overlapping section (by load order)
// settled other places the other way round (a seam down a chapter 1 cliff). The towers' chunk is known by its header
// ("KMD", its size 0x4FF470 at +0xC: the same wherever it loads), a draw's by its vertex buffer's header (the loaded
// chunk whose data holds it).
constexpr uint32_t kChunkTable = 0x9504C4, kChunkSlot = 0x14, kChunkSlots = 64;
int SectionPull() {
    static const int n = std::getenv("NGB_D3D_SECTION_PULL") ? std::atoi(std::getenv("NGB_D3D_SECTION_PULL")) : 2;
    return n;
}
float SectionSlope() {   // NGB_D3D_SECTION_SLOPE=f: console pixels' worth of depth slope (0.5)
    static const float f = std::getenv("NGB_D3D_SECTION_SLOPE") ? float(std::atof(std::getenv("NGB_D3D_SECTION_SLOPE"))) : 0.5f;
    return f;
}
// the drawbridge towers' chunk as loaded ("KMD", its size 0x4FF470 at +0xC), as a range: its data to the next loaded
// chunk's (or the end of memory), 0..0 when not loaded -- a draw's vertex buffer header in it (the loaded chunk whose
// data holds it) for every draw, from one read of the table a frame
void TowersRange(uint32_t& lo, uint32_t& hi) {
    lo = hi = 0;
    for (uint32_t i = 0; i < kChunkSlots; i++) {
        uint32_t e = kChunkTable - 4 + i * kChunkSlot, base = kernel::R32(e);
        if (kernel::R32(e + 4) == 1 && base && kernel::R32(base) == 0x00444D4B && kernel::R32(base + 0xC) == 0x004FF470)
            lo = base;
    }
    if (!lo) return;
    hi = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < kChunkSlots; i++) {
        uint32_t e = kChunkTable - 4 + i * kChunkSlot, base = kernel::R32(e);
        if (kernel::R32(e + 4) == 1 && base > lo && base < hi) hi = base;
    }
}
// the chunk slot of the map piece being drawn, or ~0u
uint32_t DrawChunkSlot() {
    if (g_draw_node < 0x10000 || g_draw_node >= 0x4000000) return ~0u;
    uint32_t g = kernel::R32(g_draw_node + 8);
    if (g < kChunkTable || g >= kChunkTable + kChunkSlot * kChunkSlots || (g - kChunkTable) % kChunkSlot) return ~0u;
    return (g - kChunkTable) / kChunkSlot;
}

// A 2D quad above 1x drawn as the console covers and samples it (gpu::QuadHlsl, the pixel shader's clamp): for an
// axis-aligned pretransformed 4-vertex quad, each stage's texture coordinate gradients in console pixels and the
// range the console's pixel centres sample -- the quad's own range less half a console pixel's worth at each side.
// Off (qsnap.x 0) at 1x, for other draws, and for a quad whose coordinates are generated or transformed.
void VulkanBackend::QuadSnap(const State& s, const Assembled& a, uint32_t count, bool quad, uint32_t samples,
                             DrawConstants& c) {
    std::memset(c.uvc, 0, sizeof(c.uvc));
    std::memset(c.uvg, 0, sizeof(c.uvg));
    std::memset(c.qsnap, 0, sizeof(c.qsnap));
    static const bool off = std::getenv("NGB_D3D_NO_QUADSNAP") != nullptr;
    // above 1x, or at 1x with MSAA (whose samples an edge between pixel centres covers in part: the same strips)
    if (off || !quad || !a.pretransformed || count != 4 || (c.surf[3] <= 1.0f && samples <= 1)) return;
    // a linearly filtered texture weighted by its alpha (qsnap.y, the pixel shader's kTexLinear stages): between
    // texels, as an upscaled picture samples, a transparent texel's colour is not blended into its opaque
    // neighbour's -- the light fringe where a sprite's picture meets the clear part of its texture (the pause
    // menu's dagger above 1x, #83). At the console's pixel centres it is the same texel as before.
    c.qsnap[1] = 1.0f;
    const VertexElement& pe = a.elements[0];
    if (pe.buffer >= a.buffer_count || !a.buffers[pe.buffer].src || !a.buffers[pe.buffer].stride) return;
    const auto& pb = a.buffers[pe.buffer];
    if (pb.bytes / pb.stride < 4) return;
    float xy[4][2], x0 = 1e30f, x1 = -1e30f, y0 = 1e30f, y1 = -1e30f;
    for (int v = 0; v < 4; v++) {
        std::memcpy(xy[v], pb.src + size_t(v) * pb.stride + pe.offset, 8);
        x0 = std::min(x0, xy[v][0]); x1 = std::max(x1, xy[v][0]);
        y0 = std::min(y0, xy[v][1]); y1 = std::max(y1, xy[v][1]);
    }
    // axis-aligned: every corner on the box, within a hundredth of a pixel (the pause menu's dagger has a corner at
    // 181.4999 beside 181.5, and its body was left out, #83); each corner taken as the box's
    constexpr float kEps = 0.01f;
    auto on = [](float a, float b) { return std::abs(a - b) <= kEps; };
    for (int v = 0; v < 4; v++) {
        if (!((on(xy[v][0], x0) || on(xy[v][0], x1)) && (on(xy[v][1], y0) || on(xy[v][1], y1)))) return;
        xy[v][0] = on(xy[v][0], x0) ? x0 : x1;
        xy[v][1] = on(xy[v][1], y0) ? y0 : y1;
    }
    if (x1 - x0 < 1.0f || y1 - y0 < 1.0f) return;
    // the corners at (x0,y0), (x1,y0), (x0,y1)
    int c00 = -1, c10 = -1, c01 = -1;
    for (int v = 0; v < 4; v++) {
        if (xy[v][0] == x0 && xy[v][1] == y0) c00 = v;
        if (xy[v][0] == x1 && xy[v][1] == y0) c10 = v;
        if (xy[v][0] == x0 && xy[v][1] == y1) c01 = v;
    }
    if (c00 < 0 || c10 < 0 || c01 < 0) return;
    for (int i = 0; i < 4; i++) {
        c.uvc[i][0] = c.uvc[i][1] = -1e30f;
        c.uvc[i][2] = c.uvc[i][3] = 1e30f;
        uint32_t set = s.tss[size_t(i)][kTssTexCoordIndex] & 3;   // pretransformed: stage i reads this set as given
        const VertexElement& te = a.elements[9 + set];
        if (!((a.regs >> (9 + set)) & 1) || te.buffer >= a.buffer_count || !a.buffers[te.buffer].src) continue;
        const auto& tb = a.buffers[te.buffer];
        if (!tb.stride || tb.bytes / tb.stride < 4) continue;
        float uv[4][2];
        for (int v = 0; v < 4; v++) std::memcpy(uv[v], tb.src + size_t(v) * tb.stride + te.offset, 8);
        float w = x1 - x0, h = y1 - y0;
        float dudx = (uv[c10][0] - uv[c00][0]) / w, dvdx = (uv[c10][1] - uv[c00][1]) / w;
        float dudy = (uv[c01][0] - uv[c00][0]) / h, dvdy = (uv[c01][1] - uv[c00][1]) / h;
        c.uvg[i][0] = dudx; c.uvg[i][1] = dudy; c.uvg[i][2] = dvdx; c.uvg[i][3] = dvdy;
        // the console samples the centres of the pixels inside: half a pixel in from each edge
        float u0 = 1e30f, u1 = -1e30f, v0 = 1e30f, v1 = -1e30f;
        for (int v = 0; v < 4; v++) {
            float cx = xy[v][0] == x0 ? 0.5f : -0.5f, cy = xy[v][1] == y0 ? 0.5f : -0.5f;
            float u = uv[v][0] + dudx * cx + dudy * cy, vv = uv[v][1] + dvdx * cx + dvdy * cy;
            u0 = std::min(u0, u); u1 = std::max(u1, u); v0 = std::min(v0, vv); v1 = std::max(v1, vv);
        }
        c.uvc[i][0] = u0; c.uvc[i][1] = v0; c.uvc[i][2] = u1; c.uvc[i][3] = v1;
    }
    c.qsnap[0] = 1.0f;
}

void VulkanBackend::Retire(Image& img, VkImageView extra) {
    graveyard_.push_back({img, extra, frame_});
    img = Image{};
}

VkDeviceSize VulkanBackend::Upload(const void* data, VkDeviceSize bytes, VkDeviceSize align) {
    const VkDeviceSize half = kRingSize / 2, base = VkDeviceSize(slot_) * half;
    VkDeviceSize at = (ring_pos_ + align - 1) & ~(align - 1);
    if (at + bytes > half) {
        Submit();
        at = 0;
    }
    if (data) std::memcpy(ring_cpu_ + base + at, data, bytes);
    ring_pos_ = at + bytes;
    return base + at;
}

bool VulkanBackend::CreateImage(Image& img, VkFormat fmt, uint32_t w, uint32_t h, uint32_t layers, uint32_t levels,
                                bool depth, bool cube, VkImageUsageFlags usage, uint32_t samples, bool mutable_format) {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.flags = (cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0) | (mutable_format ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT : 0);
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = fmt;
    ci.extent = {w, h, 1};
    ci.mipLevels = levels;
    ci.arrayLayers = layers;
    ci.samples = VkSampleCountFlagBits(samples);
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = usage;
    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    VKCHECK(vmaCreateImage(vma_, &ci, &aci, &img.image, &img.mem, nullptr));
    VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vci.image = img.image;
    vci.viewType = cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    vci.format = fmt;
    vci.subresourceRange = {VkImageAspectFlags(depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT), 0,
                            levels, 0, layers};
    VKCHECK(d_.vkCreateImageView(dev_, &vci, nullptr, &img.view));
    img.format = fmt;
    img.w = w;
    img.h = h;
    img.layers = layers;
    img.levels = levels;
    img.depth = depth;
    img.samples = samples;
    img.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    return true;
}

void VulkanBackend::DestroyImage(Image& img) {
    if (img.view) d_.vkDestroyImageView(dev_, img.view, nullptr);
    if (img.image) vmaDestroyImage(vma_, img.image, img.mem);
    img = Image{};
}

void VulkanBackend::Barrier(Image& img, VkImageLayout to) {
    if (img.layout == to) return;
    // A layout change is not allowed inside a rendering pass (dynamic rendering without local read):
    // the pass ends first, the next draw begins it again. A draw sampling a render target other than
    // the one bound (the water's screen copy, the haze's) changed its layout inside the pass still
    // open from the draw before (the validation layer's VUID 09553); RADV let it through, what
    // another driver makes of it is not defined (#16).
    if (rendering_) EndRendering();
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    b.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    b.oldLayout = img.layout;
    b.newLayout = to;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img.image;
    VkImageAspectFlags aspect = img.depth ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                                          : VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange = {aspect, 0, img.levels, 0, img.layers};
    VkDependencyInfo di{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    di.imageMemoryBarrierCount = 1;
    di.pImageMemoryBarriers = &b;
    d_.vkCmdPipelineBarrier2(cmd_, &di);
    img.layout = to;
}

// targets, by guest memory and kind
Target* VulkanBackend::EnsureTarget(const Surface& s, bool depth) {
    if (!s.header || !s.data) return nullptr;
    uint32_t w, h;
    SurfaceSize(s, w, h);
    if (!w || !h) return nullptr;
    // the render scale applies to display-sized targets, and to the medium ones the screen effects draw
    // into (256x128 and up: the sword's trail, the transitions, glows), else those stayed at the console's
    // resolution, blocky at a high render scale (#37, found by HammerD72); colour and depth alike, so an
    // effect's depth buffer matches its picture. A small depth-only target is a shadow map (the title
    // renders Ryu's shadow into 128x128) at the shadow scale; other small targets (the scarf simulation
    // renders into little textures it then samples by the texel) stay 1:1. NGB_D3D_EFFECT_SCALE=0: the
    // effect targets 1:1, for comparing.
    static const bool effect_scale = [] {
        const char* e = std::getenv("NGB_D3D_EFFECT_SCALE");
        return !e || std::strcmp(e, "0") != 0;
    }();
    bool big = w >= 640 && h >= 448;
    bool effect = effect_scale && !big && w >= 256 && h >= 128;
    uint32_t scale = big ? uint32_t(scale_)
                   : effect ? std::max(1u, std::min(uint32_t(scale_), 8192u / std::max(w, h)))
                   : (depth && w <= 512 && h <= 512) ? uint32_t(shadow_) : 1;
    if (!depth && w==128 && h==128 && ContinuousGlowEnabled() && glow_targets_.count(s.data))
        scale=GlowTargetScale(uint32_t(scale_));
    uint32_t iw = w * scale, ih = h * scale;
    uint64_t key = uint64_t(s.data) << 1 | (depth ? 1 : 0);
    auto it = targets_.find(key);
    // MSAA (a quality setting) for the display-sized targets: the 3D and what is drawn over it
    uint32_t samples = big ? uint32_t(msaa_) : 1;
    if (it != targets_.end()) {
        if (it->second.gw == w && it->second.gh == h && it->second.img.w == iw && it->second.img.h == ih &&
            it->second.samples == samples)
            return &it->second;
        if (bound_rt_ == &it->second || bound_ds_ == &it->second) EndRendering();
        Target old = it->second;   // its picture goes on into the new images (a scale or MSAA change)
        targets_.erase(it);
        Target* t = MakeTarget(s, depth, key, w, h, iw, ih, samples);
        if (t && old.drawn && !depth && old.gw == w && old.gh == h) {
            Barrier(old.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            Barrier(t->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkImageBlit b{};
            b.srcSubresource = b.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            b.srcOffsets[1] = {int32_t(old.img.w), int32_t(old.img.h), 1};
            b.dstOffsets[1] = {int32_t(t->img.w), int32_t(t->img.h), 1};
            d_.vkCmdBlitImage(cmd_, old.img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, t->img.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &b, VK_FILTER_LINEAR);
            t->drawn = true;
            t->ms_stale = t->samples > 1;
        }
        Submit();
        if (shared_) nv2a_vk_queue_lock();
        d_.vkDeviceWaitIdle(dev_);   // the presenter may still be reading the old one
        if (shared_) nv2a_vk_queue_unlock();
        DestroyImage(old.img);
        DestroyImage(old.ms);
        DestroyImage(old.fb);
        return t;
    }
    return MakeTarget(s, depth, key, w, h, iw, ih, samples);
}

Target* VulkanBackend::MakeTarget(const Surface&, bool depth, uint64_t key, uint32_t w, uint32_t h, uint32_t iw,
                                  uint32_t ih, uint32_t samples) {
    Target t;
    t.gw = w;
    t.gh = h;
    t.samples = samples;
    t.addr = uint32_t(key >> 1);
    VkImageUsageFlags u = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                          (depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    if (!CreateImage(t.img, depth ? ds_format_ : kColorFormat, iw, ih, 1, 1, depth, false, u)) return nullptr;
    if (samples > 1 &&
        !CreateImage(t.ms, depth ? ds_format_ : kColorFormat, iw, ih, 1, 1, depth, false,
                     depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, samples)) {
        DestroyImage(t.img);
        return nullptr;
    }
    return &(targets_[key] = t);
}

void VulkanBackend::FillMs(Target& t) {
    static const uint64_t kVs = 0xF1110000F1110001ull, kPs = 0xF1110000F1110002ull;
    Module(kVs, [] {
        return std::string("struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "VO main(uint id : SV_VertexID) {\n"
                           "    VO o; float2 p = float2((id << 1) & 2, id & 2);\n"
                           "    o.uv = p; o.pos = float4(p * 2 - 1, 0, 1); return o;\n}\n");
    }, true);
    Module(kPs, [] {
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                           "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "float4 main(VO i) : SV_Target { return tex0.Sample(smp0, i.uv); }\n");
    }, false);
    PipeKey k;
    std::memset(&k, 0, sizeof(k));
    k.vs = kVs;
    k.ps = kPs;
    k.topo = 2;
    k.wmask = 0xF;
    k.has_rt = 1;
    k.samples = t.samples;
    VkPipeline pipe = Pipeline(k);
    if (!fill_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter = ci.minFilter = VK_FILTER_NEAREST;
        ci.addressModeU = ci.addressModeV = ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        d_.vkCreateSampler(dev_, &ci, nullptr, &fill_sampler_);
    }
    if (!pipe || !fill_sampler_) return;
    Barrier(t.img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(t.ms, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = t.ms.view;
    ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {t.ms.w, t.ms.h}};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    VkViewport vpt{0, 0, float(t.ms.w), float(t.ms.h), 0, 1};
    VkRect2D sc{{0, 0}, {t.ms.w, t.ms.h}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vpt);
    d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im{VK_NULL_HANDLE, t.img.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{fill_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[2] = {{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}};
    w[0].dstBinding = 2;
    w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    w[0].pImageInfo = &im;
    w[1].dstBinding = 6;
    w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    w[1].pImageInfo = &sm;
    d_.vkCmdPushDescriptorSetKHR(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 2, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
}

void VulkanBackend::VisibilityEnd(uint32_t report_va) {
    vis_on_ = false;
    vis_end_[slot_].push_back({vis_test_, report_va});
}

// The slot's frame is done: its queries' counts, and the reports of the tests that ended in it -- laid out as
// the GPU writes them (xemu's pgraph_write_zpass_pixel_cnt_report): a timestamp, the count, 0.
void VulkanBackend::VisResults(int slot) {
    if (!vis_q_[slot].empty()) {
        std::vector<uint64_t> r(vis_q_[slot].size());
        if (d_.vkGetQueryPoolResults(dev_, vis_pool_[slot], 0, uint32_t(r.size()), r.size() * 8, r.data(), 8,
                                     VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS)
            for (size_t i = 0; i < r.size(); i++) vis_sum_[vis_q_[slot][i].test] += double(r[i]) / vis_q_[slot][i].div;
        vis_q_[slot].clear();
    }
    for (const auto& e : vis_end_[slot]) {
        auto it = vis_sum_.find(e.first);
        uint32_t count = it == vis_sum_.end() ? 0 : uint32_t(std::lround(it->second));
        if (it != vis_sum_.end()) vis_sum_.erase(it);
        uint8_t* p = static_cast<uint8_t*>(HostPtr(e.second));
        uint64_t stamp = 0x0011223344556677ull;
        uint32_t done = 0;
        std::memcpy(p, &stamp, 8);
        std::memcpy(p + 8, &count, 4);
        std::memcpy(p + 12, &done, 4);
        static const bool log = std::getenv("NGB_VIS_LOG") != nullptr;
        if (log) std::fprintf(stderr, "[VIS] test %u: %u pixels -> %08X\n", e.first, count, e.second);
    }
    vis_end_[slot].clear();
    if (vis_sum_.size() > 256) vis_sum_.clear();   // tests begun and never ended
}


// Recognize the complete current-frame chain before enlarging its final
// colour target. A shadow, simulation or unrelated 128x128 target keeps
// its original allocation. The classification persists through its clear.
void VulkanBackend::ClassifyGlowTarget(const State& s,const Assembled& a,uint32_t count) {
    if (!ContinuousGlowEnabled() || ContinuousGlowStage(s,a,count)!=2) return;
    glow_targets_.insert(s.rt.data);
}

#include "glow_filter.h"

void VulkanBackend::SetGamma(const uint8_t* ramp) {
    if (!ramp) return;
    std::memcpy(gamma_, ramp, sizeof(gamma_));   // red[256], green[256], blue[256]
    bool identity = true;
    for (int c = 0; c < 3 && identity; c++)
        for (int i = 0; i < 256; i++)
            if (gamma_[c * 256 + i] != i) { identity = false; break; }
    gamma_on_ = !identity;
    gamma_dirty_ = true;
    if (std::getenv("NGB_D3D_GAMMALOG"))
        std::fprintf(stderr, "[D3D] gamma ramp %s: r %u %u %u g %u %u %u b %u %u %u\n", identity ? "identity" : "set",
                     gamma_[0], gamma_[128], gamma_[255], gamma_[256], gamma_[384], gamma_[511], gamma_[512],
                     gamma_[640], gamma_[767]);
}

// src through the gamma ramp into dst (the same size): a full screen pass looking each channel up.
void VulkanBackend::GammaCopy(Image& src, Image& dst, float bars, float bars_y) {
    if (!gamma_lut_.image) {
        CreateImage(gamma_lut_, kColorFormat, 256, 1, 1, 1, false, false,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        gamma_dirty_ = true;
    }
    if (gamma_dirty_) {
        uint8_t texels[256 * 4];
        for (int i = 0; i < 256; i++) {          // B G R A
            texels[i * 4] = gamma_[512 + i];
            texels[i * 4 + 1] = gamma_[256 + i];
            texels[i * 4 + 2] = gamma_[i];
            texels[i * 4 + 3] = 255;
        }
        VkDeviceSize at = Upload(texels, sizeof(texels), 16);
        Barrier(gamma_lut_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy c{};
        c.bufferOffset = at;
        c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        c.imageExtent = {256, 1, 1};
        d_.vkCmdCopyBufferToImage(cmd_, ring_, gamma_lut_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
        gamma_dirty_ = false;
    }
    uint32_t edge = uint32_t(std::lround(std::clamp(bars, 0.0f, 0.49f) * 10000));
    uint32_t edge_y = uint32_t(std::lround(std::clamp(bars_y, 0.0f, 0.49f) * 10000));
    const uint64_t kVs = 0xF1110000F1110001ull,
                   kPs = 0xF1110000F1120000ull | uint64_t(gamma_on_) << 16 | 1ull << 17 | edge | uint64_t(edge_y) << 20 |
                         uint64_t(edge_use_ && edge_.image) << 18;
    const bool fill = edge_use_ && edge_.image;
    const std::string bar = fill ? "" : "    return float4(0, 0, 0, 1);\n";
    // both side bars in the right edge's colours: the same on both sides looks better, and the menus' left
    // edges carry ornaments (the pause menu's dragon and thorns) where the right ones have their background
    Module(kVs, [] {
        return std::string("struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "VO main(uint id : SV_VertexID) {\n"
                           "    VO o; float2 p = float2((id << 1) & 2, id & 2);\n"
                           "    o.uv = p; o.pos = float4(p * 2 - 1, 0, 1); return o;\n}\n");
    }, true);
    Module(kPs, [&] {
        char e[48], ey[48];
        std::snprintf(e, sizeof e, "%.4f", edge / 10000.0);
        std::snprintf(ey, sizeof ey, "%.4f", edge_y / 10000.0);
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                           "Texture2D tex1 : register(t1);\nSamplerState smp1 : register(s1);\n"
                           "Texture2D tex2 : register(t2);\nSamplerState smp2 : register(s2);\n"
                           "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "float ramp(float v, int c) {\n"
                           "    float4 t = tex1.SampleLevel(smp1, float2((round(saturate(v) * 255) + 0.5) / 256, 0.5), 0);\n"
                           "    return c == 0 ? t.r : c == 1 ? t.g : t.b;\n}\n"
                           "float4 main(VO i) : SV_Target {\n"
                           "    float4 c = tex0.Sample(smp0, i.uv);\n") +
               (edge ? std::string("    if (i.uv.x < ") + e + ") {\n" + bar +
                           "        return tex2.SampleLevel(smp2, float2(i.uv.y, 0.375), 0);\n    }\n"
                           "    if (i.uv.x > 1 - " + e + ") {\n" + bar +
                           "        return tex2.SampleLevel(smp2, float2(i.uv.y, 0.375), 0);\n    }\n"
                     : std::string()) +
               (edge_y ? std::string("    if (i.uv.y < ") + ey + ") {\n" + bar +
                             "        return tex2.SampleLevel(smp2, float2(i.uv.x, 0.625), 0);\n    }\n"
                             "    if (i.uv.y > 1 - " + ey + ") {\n" + bar +
                             "        return tex2.SampleLevel(smp2, float2(i.uv.x, 0.875), 0);\n    }\n"
                       : std::string()) +
               (gamma_on_ ? "    return float4(ramp(c.r, 0), ramp(c.g, 1), ramp(c.b, 2), c.a);\n}\n" : "    return c;\n}\n");
    }, false);
    PipeKey k;
    std::memset(&k, 0, sizeof(k));
    k.vs = kVs;
    k.ps = kPs;
    k.topo = 2;
    k.wmask = 0xF;
    k.has_rt = 1;
    k.samples = 1;
    VkPipeline pipe = Pipeline(k);
    if (!fill_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter = ci.minFilter = VK_FILTER_NEAREST;
        ci.addressModeU = ci.addressModeV = ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        d_.vkCreateSampler(dev_, &ci, nullptr, &fill_sampler_);
    }
    if (!pipe || !fill_sampler_) return;
    Barrier(src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(gamma_lut_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(fill ? edge_ : null2d_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);   // before the pass: no layout change in it
    Barrier(dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = dst.view;
    ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {dst.w, dst.h}};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    VkViewport vpt{0, 0, float(dst.w), float(dst.h), 0, 1};
    VkRect2D sc{{0, 0}, {dst.w, dst.h}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vpt);
    d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im0{VK_NULL_HANDLE, src.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo im1{VK_NULL_HANDLE, gamma_lut_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{fill_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkDescriptorImageInfo im2{VK_NULL_HANDLE, fill ? edge_.view : null2d_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm2{edge_sampler_ ? edge_sampler_ : fill_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[6] = {};
    for (int i = 0; i < 6; i++) {
        w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w[i].descriptorCount = 1;
    }
    w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im0;
    w[1].dstBinding = 3; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[1].pImageInfo = &im1;
    w[2].dstBinding = 6; w[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[2].pImageInfo = &sm;
    w[3].dstBinding = 7; w[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[3].pImageInfo = &sm;
    w[4].dstBinding = 4; w[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[4].pImageInfo = &im2;
    w[5].dstBinding = 8; w[5].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[5].pImageInfo = &sm2;
    d_.vkCmdPushDescriptorSetKHR(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 6, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
}

#include "quick_menu.inc"

// Into edge_ (16 x 4: bands of the left, right, top and bottom edge), the picture's colours just inside
// its 16:9 part, each band an average of 32 points, blended in at 4% a frame: the fill follows a new
// screen (or a menu that fades in) within a second, too slowly for anything animated to flicker in it.
void VulkanBackend::EdgeSample(Image& src, float bars, float bars_y) {
    edge_frames_++;
    if (!edge_.image &&
        !CreateImage(edge_, kColorFormat, 16, 4, 1, 1, false, false,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
        return;
    if (!edge_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter = ci.minFilter = VK_FILTER_LINEAR;
        ci.addressModeU = ci.addressModeV = ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        d_.vkCreateSampler(dev_, &ci, nullptr, &edge_sampler_);
    }
    if (!fill_sampler_ || !edge_sampler_) return;
    uint32_t bx = uint32_t(std::lround(std::clamp(bars, 0.0f, 0.49f) * 10000));
    uint32_t by = uint32_t(std::lround(std::clamp(bars_y, 0.0f, 0.49f) * 10000));
    const uint64_t kVs = 0xF1110000F1110001ull, kPs = 0xF1130000F1130000ull | bx | uint64_t(by) << 20;
    Module(kVs, [] {
        return std::string("struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "VO main(uint id : SV_VertexID) {\n"
                           "    VO o; float2 p = float2((id << 1) & 2, id & 2);\n"
                           "    o.uv = p; o.pos = float4(p * 2 - 1, 0, 1); return o;\n}\n");
    }, true);
    Module(kPs, [&] {
        char b[160];
        std::snprintf(b, sizeof b, "static const float BX = %.4f, BY = %.4f, IN = 0.05;\n", bx / 10000.0, by / 10000.0);
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n") + b +
               "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
               "float4 main(VO i) : SV_Target {\n"
               "    float band = floor(i.uv.x * 16), side = floor(i.uv.y * 4);\n"
               "    float3 sum = 0;\n"
               "    for (int a = 0; a < 4; a++) for (int c = 0; c < 8; c++) {\n"
               "        float across = (a + 0.5) / 4 * IN, along = (band + (c + 0.5) / 8) / 16;\n"
               "        float2 uv = side == 0 ? float2(BX + across, along) : side == 1 ? float2(1 - BX - across, along)\n"
               "                  : side == 2 ? float2(along, BY + across) : float2(along, 1 - BY - across);\n"
               "        sum += tex0.SampleLevel(smp0, uv, 0).rgb;\n"
               "    }\n"
               "    return float4(sum / 32, 1);\n}\n";
    }, false);
    PipeKey k;
    std::memset(&k, 0, sizeof(k));
    k.vs = kVs;
    k.ps = kPs;
    k.topo = 2;
    k.wmask = 0xF;
    k.has_rt = 1;
    k.samples = 1;
    k.blend = 1;
    k.src = 0x8003;   // constant alpha: a share of the new
    k.dst = 0x8004;
    k.op = 0x8006;
    VkPipeline pipe = Pipeline(k);
    if (!pipe) return;
    Barrier(src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    bool first = edge_frames_ == 1;
    Barrier(edge_, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = edge_.view;
    ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {16, 4}};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    VkViewport vpt{0, 0, 16, 4, 0, 1};
    VkRect2D sc{{0, 0}, {16, 4}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vpt);
    d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    const float bc[4] = {0, 0, 0, first ? 1.0f : 0.04f};   // the first frame: as it is
    d_.vkCmdSetBlendConstants(cmd_, bc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im0{VK_NULL_HANDLE, src.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{edge_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[2] = {};
    for (int i = 0; i < 2; i++) {
        w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w[i].descriptorCount = 1;
    }
    w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im0;
    w[1].dstBinding = 6; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[1].pImageInfo = &sm;
    d_.vkCmdPushDescriptorSetKHR(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 2, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
}

void VulkanBackend::EndRendering() {
    if (!rendering_) return;
    d_.vkCmdEndRendering(cmd_);
    rendering_ = false;
}

// The CPU's writes to the start of a target's memory, counted (nv2a_dirty_stamp): the GPU draws only into the
// images here, never into guest memory, so a change since the last pass into it is the game putting something
// else there -- a texture loaded where a target was, and that target left behind is stale (#81)
bool VulkanBackend::TargetStamp(const Target& t, uint64_t& stamp) {
    size_t len = std::min<size_t>(size_t(t.gw) * t.gh * 2, 64 << 10);
    return len && nv2a_dirty_stamp(HostPtr(t.addr | 0x80000000u), len, &stamp) != 0;
}

void VulkanBackend::BeginRendering(Target* rt, Target* ds) {
    // a colour and a depth target of different sizes cannot share a pass (one scaled, one not):
    // the depth goes without, as the title's picture is in the colour
    if (rt && ds && (rt->img.w != ds->img.w || rt->img.h != ds->img.h || rt->samples != ds->samples)) ds = nullptr;
    if (rendering_ && rt == bound_rt_ && ds == bound_ds_) return;
    EndRendering();
    if (rt && rt->ms_stale) {
        if (rt->drawn) FillMs(*rt);
        rt->ms_stale = false;
    }
    bound_rt_ = rt;
    bound_ds_ = ds;
    if (rt) Barrier(rt->img, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    if (ds) Barrier(ds->img, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    if (rt && rt->samples > 1) Barrier(rt->ms, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    if (ds && ds->samples > 1) Barrier(ds->ms, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    {   // The pass before, on the same targets in the same layout, got no barrier above (Barrier skips an
        // unchanged layout), and Vulkan orders attachment writes only within a pass: its depth and colour
        // writes against this pass's tests and blending need one. RADV keeps passes in order regardless;
        // AMD's Windows driver does not have to -- Ryu's shadow, drawn in a pass after another on its shadow
        // map, went missing for a few frames at a time there (#50).
        VkMemoryBarrier2 mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        mb.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                          VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        mb.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        mb.dstStageMask = mb.srcStageMask;
        mb.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                           VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                           VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkDependencyInfo di{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        di.memoryBarrierCount = 1;
        di.pMemoryBarriers = &mb;
        static const bool off = std::getenv("NGB_D3D_NO_PASS_BARRIER") != nullptr;   // to compare
        if (!off) d_.vkCmdPipelineBarrier2(cmd_, &di);
    }
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo da{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    if (rt) {
        ca.imageView = rt->img.view;
        ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ca.loadOp = rt->drawn ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (rt->samples > 1) {                       // drawn multisampled, resolved as the pass ends
            ca.imageView = rt->ms.view;
            ca.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            ca.resolveImageView = rt->img.view;
            ca.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        rt->drawn = true;
        StampTarget(*rt);
    }
    if (ds) {
        da.imageView = ds->img.view;
        da.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        da.loadOp = ds->drawn ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        da.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        da.clearValue.depthStencil = {1.0f, 0};
        if (ds->samples > 1) {
            da.imageView = ds->ms.view;
            da.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
            da.resolveImageView = ds->img.view;
            da.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        ds->drawn = true;
        StampTarget(*ds);
    }
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    const Image& any = rt ? rt->img : ds->img;
    ri.renderArea = {{0, 0}, {any.w, any.h}};
    ri.layerCount = 1;
    ri.colorAttachmentCount = rt ? 1 : 0;
    ri.pColorAttachments = rt ? &ca : nullptr;
    ri.pDepthAttachment = ds ? &da : nullptr;
    ri.pStencilAttachment = ds ? &da : nullptr;
    d_.vkCmdBeginRendering(cmd_, &ri);
    rendering_ = true;
}

void VulkanBackend::Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color,
                          float z, uint32_t stencil) {
    if (!pk_given_) CapturePeek(s, pk_);
    if (ContinuousGlowEnabled() && (flags & kClearTarget)) InvalidateContinuousGlow(s.rt.data);
    Target* rt = (flags & kClearTarget) ? EnsureTarget(s.rt, false) : nullptr;
    Target* ds = (flags & (kClearZ | kClearStencil)) ? EnsureTarget(s.zs, true) : nullptr;
    if (!rt && !ds) return;
    // the targets as the draws would bind them, so the clear lands in the same pass
    Target* brt = EnsureTarget(s.rt, false);
    Target* bds = s.zs.header ? EnsureTarget(s.zs, true) : nullptr;
    if (!brt && !bds) return;
    BeginRendering(brt, bds);
    const Image& any = brt ? brt->img : bds->img;
    uint32_t gw = brt ? brt->gw : bds->gw, gh = brt ? brt->gh : bds->gh;
    float sx = float(any.w) / gw, sy = float(any.h) / gh;
    std::vector<VkClearRect> rs;
    auto add = [&](int x0, int y0, int x1, int y1) {
        x0 = std::clamp(int(x0 * sx), 0, int(any.w));
        y0 = std::clamp(int(y0 * sy), 0, int(any.h));
        x1 = std::clamp(int(x1 * sx), 0, int(any.w));
        y1 = std::clamp(int(y1 * sy), 0, int(any.h));
        if (x1 > x0 && y1 > y0) rs.push_back({{{x0, y0}, {uint32_t(x1 - x0), uint32_t(y1 - y0)}}, 0, 1});
    };
    if (count && rects) {
        for (uint32_t i = 0; i < count; i++) {
            int32_t r[4];
            std::memcpy(r, rects + i * 16, 16);
            add(r[0], r[1], r[2], r[3]);
        }
    } else {
        const auto& v = s.viewport;
        add(int(v.x), int(v.y), int(v.x + v.w), int(v.y + v.h));
    }
    if (rs.empty()) return;
    VkClearAttachment at[2];
    uint32_t n = 0;
    if (rt && rt == brt) {
        at[n] = {VK_IMAGE_ASPECT_COLOR_BIT, 0, {}};
        Color(color, at[n].clearValue.color.float32);
        n++;
    }
    if (ds && ds == bds) {
        VkImageAspectFlags a = ((flags & kClearZ) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
                               ((flags & kClearStencil) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
        at[n] = {a, 0, {}};
        at[n].clearValue.depthStencil = {std::clamp(z, 0.0f, 1.0f), stencil & 0xFF};
        n++;
    }
    if (trace_)
        std::fprintf(trace_, "%u clear rt %08X flags %X color %08X rects %u n %u\n", presents_, s.rt.data, flags, color, count, n);
    if (n) d_.vkCmdClearAttachments(cmd_, n, at, uint32_t(rs.size()), rs.data());
}

// a texture stage: a target the title draws into and reads back, or a texture from guest memory
VkImageView VulkanBackend::BindTexture(const State& s, int stage, TexInfo& info, bool& from_target) {
    info = TexInfo{};
    from_target = false;
    uint32_t hdr = s.textures[size_t(stage)];
    TextureDesc d;
    if (!hdr || !Describe(hdr, d)) return VK_NULL_HANDLE;
    const FormatInfo& fi = Format(d.format);
    info.kind = d.cube ? kTexCube : d.depth > 1 ? kTex3D : kTex2D;
    info.rect = d.linear;
    // a render target read as a texture
    // (targets are keyed by the surface's physical address, the description has the virtual one)
    auto tit = targets_.find(uint64_t(d.data & 0x7FFFFFFFu) << 1 | (fi.depth ? 1 : 0));
    // A bump map (V8U8: nothing renders into that) or a mip chain is never a target: the game has put a
    // texture where it once drew, and the target left behind is not it (#16: the pond's ripple maps,
    // read as the target that memory had held, a picture of something else).
    // DXT and P8 are never drawn into either (#81: DXT backdrops read as the target once there, black).
    bool never_target = d.format == 0x28 || d.format == 0x17 || d.levels > 1 || fi.compressed || d.format == 0x0B;
    // And a target whose memory the CPU has written since it was last drawn into holds a texture now (#81):
    // the target goes, and the texture is read from memory.
    static const bool keep_stale = std::getenv("NGB_D3D_KEEP_STALE_TARGETS") != nullptr;
    if (tit != targets_.end() && !keep_stale && tit->second.stamped && &tit->second != draw_rt_ &&
        &tit->second != bound_rt_ && &tit->second != bound_ds_) {
        uint64_t now;
        if (TargetStamp(tit->second, now) && now != tit->second.stamp) {
            Target& t = tit->second;
            static int told;
            if (told < 32 && ++told)
                std::fprintf(stderr, "[D3D] target @%08X %ux%u: memory rewritten since drawn, read as texture fmt %X %ux%u\n",
                             t.addr, t.gw, t.gh, d.format, d.width, d.height);
            Retire(t.img);
            if (t.ms.image) Retire(t.ms);
            if (t.fb.image) Retire(t.fb);
            targets_.erase(tit);
            tit = targets_.end();
        }
    }
    if (tit != targets_.end() && !d.cube && d.depth == 1 && never_target) {
        static std::set<uint32_t> told;
        if (told.size() < 32 && told.insert(d.data).second)
            std::fprintf(stderr, "[D3D] texture @%08X fmt %X %ux%u levels %u: a target's memory (%ux%u), read as the texture\n",
                         d.data, d.format, d.width, d.height, d.levels, tit->second.gw, tit->second.gh);
    } else if (tit != targets_.end() && !d.cube && d.depth == 1) {
        Target& t = tit->second;
        if ((d.width != t.gw || d.height != t.gh) && told_size_.size() < 32 && told_size_.insert(d.data).second)
            std::fprintf(stderr, "[D3D] texture @%08X fmt %X %ux%u reads a target of %ux%u\n", d.data, d.format, d.width,
                         d.height, t.gw, t.gh);
        if (&t == bound_rt_ || &t == bound_ds_) EndRendering();
        if (&t == draw_rt_ && !t.img.depth) {
            // The draw reads the target it draws into (a glow or water pass building on its last picture):
            // Vulkan leaves sampling an image while it is the colour attachment undefined (validation VUID
            // 00344; AMD read the old picture, other drivers need not). It reads a copy of the picture as
            // it is before the draw, which is what such a pass can count on the console too.
            if (!t.fb.image && told_size_.size() < 64 && told_size_.insert(d.data | 1).second)
                std::fprintf(stderr, "[D3D] target @%08X %ux%u read by a draw into it: from a copy\n", d.data, t.gw, t.gh);
            if (t.fb.image && (t.fb.w != t.img.w || t.fb.h != t.img.h)) Retire(t.fb);
            if (!t.fb.image && !CreateImage(t.fb, t.img.format, t.img.w, t.img.h, 1, 1, false, false,
                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, false))
                return VK_NULL_HANDLE;
            Barrier(t.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            Barrier(t.fb, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkImageCopy c{};
            c.srcSubresource = c.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            c.extent = {t.img.w, t.img.h, 1};
            d_.vkCmdCopyImage(cmd_, t.img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, t.fb.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
            Barrier(t.fb, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            from_target = true;
            return t.fb.view;
        }
        if (t.img.depth) info.shadow = (d.format == 0x2C || d.format == 0x30) ? 2 : 1;
        static const bool shbar = std::getenv("NGB_D3D_SHADOW_BARRIER") != nullptr;   // #50 test: everything before, done
        if (shbar && t.img.depth) {
            EndRendering();
            VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            d_.vkCmdPipelineBarrier(cmd_, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb,
                                    0, nullptr, 0, nullptr);
        }
        Barrier(t.img, t.img.depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        from_target = true;
        return t.img.view;
    }
    if (fi.host == Host::None || info.kind == kTex3D) return VK_NULL_HANDLE;
    // the texture's bytes and palette
    uint32_t bytes = TotalSize(d);
    const uint8_t* guest = static_cast<const uint8_t*>(HostPtr(d.data));
    // its bytes' signature, once a frame: hashed again at every draw that used it, it was a tenth of the
    // game thread's time in busy scenes (#17); a texture rewritten between two draws of one frame is seen
    // changed from the next frame on. And hashed again only when its pages were written since: every texture
    // of a frame hashed whole each frame was the game thread's largest cost in busy scenes on older CPUs (#69).
    // The stamp comes first -- it protects the pages, so a write while hashing changes the next one.
    uint64_t sig;
    {
        uint64_t mk = uint64_t(d.data) << 32 ^ uint64_t(d.format) << 24 ^ uint64_t(d.width) << 12 ^ d.height ^
                      uint64_t(bytes) << 40;
        auto& m = sig_memo_[mk];
        if (m.frame == frame_ + 1) {
            sig = m.sig;
        } else {
            uint64_t stamp = 0;
            bool stamped = nv2a_dirty_stamp(guest, bytes, &stamp) != 0;
            static const int check = [] { const char* e = std::getenv("NGB_D3D_STAMP_CHECK"); return e ? std::atoi(e) : 0; }();
            static const bool off = [] { const char* e = std::getenv("NGB_D3D_TEX_STAMP"); return e && *e == '0'; }();
            if (off || !(stamped && m.stamped && m.stamp == stamp && m.frame)) {
                m.sig = Hash(guest, bytes, uint64_t(d.format) << 32 | d.width << 16 | d.height);
            } else if (check) {   // NGB_D3D_STAMP_CHECK=1: a texture the stamp calls unchanged, hashed anyway
                static unsigned long checked, wrong;
                uint64_t h = Hash(guest, bytes, uint64_t(d.format) << 32 | d.width << 16 | d.height);
                checked++;
                if (h != m.sig) {
                    wrong++;
                    std::fprintf(stderr, "[STAMP] changed unseen: %08X %ux%u fmt %02X (%u bytes)\n", d.data, d.width,
                                 d.height, d.format, bytes);
                    m.sig = h;
                }
                if (checked % 20000 == 0)
                    std::fprintf(stderr, "[STAMP] %lu reused, %lu changed unseen\n", checked, wrong);
            }
            sig = m.sig;
            m.frame = frame_ + 1;
            m.stamp = stamp;
            m.stamped = stamped;
        }
        if (sig_memo_.size() > 65536) sig_memo_.clear();
    }
    const uint32_t* palette = nullptr;
    if (d.format == 0x0B && s.palettes[size_t(stage)]) {
        uint32_t pal = s.palettes[size_t(stage)];
        palette = static_cast<const uint32_t*>(HostPtr(ResourceDataOf(pal)));
        sig = Hash(palette, 1024, sig);
    }
    uint64_t key = uint64_t(d.data) << 8 | d.format;
    Tex& tx = textures_[key];
    // a bump map (the next stage's texture mode 6 or 7 reads this one): V8U8, its signed bytes decoded
    // texel by texel in the shader (gpu/shaders.cpp bumpduv), not by the sampler's filter
    bool bump_src = s.ps && stage < 3 && (d.format == 0x28 || d.format == 0x17) && [&] {
        uint32_t m = (s.rs[kRsPsTextureModes] >> ((stage + 1) * 5)) & 0x1F;
        return m == 6 || m == 7;
    }();
    // Anisotropic filtering on: a texture the game gives no smaller levels gets them made, as xemu's
    // smooth filtering does -- sampled at its top level only, a detailed texture (gold leaf, rock, wood)
    // sparkles and shimmers in the distance whenever the camera moves. Opaque ones only: averaging a
    // cut-out's alpha would thin it away with distance.
    auto pow2 = [](uint32_t v) { return v && !(v & (v - 1)); };
    bool bc = fi.host == Host::BC1 || fi.host == Host::BC2 || fi.host == Host::BC3;
    bool mip_candidate = aniso_level_ > 0 && d.levels == 1 && !d.cube && (TextureFormat(fi.host) == kColorFormat || bc) &&
                         d.width >= 8 && d.height >= 8 && pow2(d.width) && pow2(d.height) &&
                         d.format != 0x28 && d.format != 0x17;
    if (tx.img.image && tx.sig == sig && tx.img.w == d.width && tx.img.h == d.height &&
        (tx.gen == 2 || tx.gen == (mip_candidate ? 1 : 0))) {
        tx.used = frame_;
        if (bump_src) {
            info.flags |= kTexSigned;
            bump_key_ = key;
        }
        return tx.img.view;
    }
    if (tx.img.image) Retire(tx.img);   // made again: the old one goes when no frame in flight uses it
    uint32_t faces = d.cube ? 6 : 1;
    VkFormat fmt = TextureFormat(fi.host);
    uint32_t levels = d.levels;
    std::vector<uint8_t> top;
    tx.gen = 0;
    static const bool miplog = std::getenv("NGB_D3D_MIPLOG") != nullptr;
    if (miplog) {   // debugging: which textures have one level, and their format
        static int n;
        if (d.levels == 1 && !d.cube && n < 300) {
            n++;
            std::fprintf(stderr, "[MIP] tex %08X fmt %X %ux%u host %d candidate %d\n", d.data, d.format, d.width, d.height,
                         int(fi.host), mip_candidate);
        }
    }
    if (mip_candidate) {
        uint32_t pitch = 0;
        DecodeLevel(d, 0, 0, guest, palette, top, pitch);
        if (bc && !top.empty()) {   // DXT: decoded here, the chain made from the texels
            std::vector<uint8_t> raw;
            raw.swap(top);
            DecodeBC(fi.host, raw.data(), d.width, d.height, top);
        }
        bool opaque = !top.empty();
        for (size_t i = 3; opaque && i < top.size(); i += 4) opaque = top[i] == 0xFF;
        if (opaque) {
            levels = 1;
            while ((std::max(d.width, d.height) >> levels) > 0) levels++;
            tx.gen = 1;
            fmt = kColorFormat;
        } else {
            tx.gen = 2;        // not to be made: the same whatever the setting
        }
    }
    auto make = [&] {
        return CreateImage(tx.img, fmt, d.width, d.height, faces, levels, false, d.cube,
                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                               (tx.gen == 1 ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0), 1, false);
    };
    if (!make()) {   // out of memory: what this frame and the last didn't use goes, and once more (#81)
        size_t gone = EvictTextures(2);
        std::fprintf(stderr, "[D3D] texture %08X %ux%u: no memory, %zu idle textures freed\n", d.data, d.width, d.height,
                     gone);
        if (!gone || !make())
            return VK_NULL_HANDLE;
    }
    tx.sig = sig;
    tx.used = frame_;
    // NGB_D3D_TEXDUMP=dir: the first textures' top level, as DDS (DXT) or PPM (BGRA), to check the data
    static const char* texdump = std::getenv("NGB_D3D_TEXDUMP");
    static int texdumped;
    if (texdump && texdumped < 40) {
        texdumped++;
        DumpTexture(d, fi, guest, palette, texdump, nullptr);
    }
    EndRendering();
    Barrier(tx.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    std::vector<uint8_t> out;
    for (uint32_t f = 0; f < faces; f++)
        for (uint32_t l = 0; l < d.levels; l++) {
            uint32_t pitch = 0;
            if (tx.gen == 1) out = top;   // the top level as decoded above (BGRA)
            else DecodeLevel(d, l, f, guest, palette, out, pitch);
            if (out.empty()) continue;
            if (bump_src && l == 0 && f == 0 && bumpcheck_) tx.check = out;
            if (bump_src && f == 0 && (l == 0 || std::getenv("NGB_D3D_BUMPLOG"))) {   // a bump map's first upload: its values, in the log (#16: they differ by machine)
                static int n;
                if (n++ < (std::getenv("NGB_D3D_BUMPLOG") ? 24 : 8)) {
                    int umin = 127, umax = -128, vmin = 127, vmax = -128;
                    for (size_t i = 0; i < out.size(); i += 4) {
                        umin = std::min(umin, int(int8_t(out[i]))); umax = std::max(umax, int(int8_t(out[i])));
                        vmin = std::min(vmin, int(int8_t(out[i + 1]))); vmax = std::max(vmax, int(int8_t(out[i + 1])));
                    }
                    std::fprintf(stderr, "[D3D] bump map @%08X %ux%u levels %u (level %u at +%X): u %d..%d v %d..%d, first bytes %02X%02X %02X%02X %02X%02X %02X%02X\n",
                                 d.data, d.width, d.height, d.levels, l, LevelOffset(d, l), umin, umax, vmin, vmax,
                                 guest[0], guest[1], guest[2], guest[3], guest[4], guest[5], guest[6], guest[7]);
                }
            }
            VkDeviceSize at = Upload(out.data(), out.size(), 16);
            VkBufferImageCopy c{};
            c.bufferOffset = at;
            c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, l, f, 1};
            c.imageExtent = {std::max(d.width >> l, 1u), std::max(d.height >> l, 1u), 1};
            d_.vkCmdCopyBufferToImage(cmd_, ring_, tx.img.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
        }
    if (tx.gen == 1) {   // the rest of the chain, each level a 2:1 linear blit (a box filter) of the one above
        VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        b.srcStageMask = b.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        b.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.image = tx.img.image;
        VkDependencyInfo di{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        di.imageMemoryBarrierCount = 1;
        di.pImageMemoryBarriers = &b;
        for (uint32_t l = 1; l < levels; l++) {
            b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, l - 1, 1, 0, 1};
            d_.vkCmdPipelineBarrier2(cmd_, &di);
            VkImageBlit bl{};
            bl.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, l - 1, 0, 1};
            bl.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, l, 0, 1};
            bl.srcOffsets[1] = {int32_t(std::max(d.width >> (l - 1), 1u)), int32_t(std::max(d.height >> (l - 1), 1u)), 1};
            bl.dstOffsets[1] = {int32_t(std::max(d.width >> l, 1u)), int32_t(std::max(d.height >> l, 1u)), 1};
            d_.vkCmdBlitImage(cmd_, tx.img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tx.img.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bl, VK_FILTER_LINEAR);
        }
        // all levels to TRANSFER_DST (the last is there already), then as one to shader reading
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, levels - 1, 0, 1};
        b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        b.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        d_.vkCmdPipelineBarrier2(cmd_, &di);
    }
    Barrier(tx.img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    tx.info = info;
    if (bump_src) {
        info.flags |= kTexSigned;
        bump_key_ = key;
        if (bumpcheck_) BumpCheck(tx, "after its upload");
    }
    return tx.img.view;
}

// NGB_D3D_BUMPCHECK=1: a bump map's top level read back from the GPU and compared with the bytes that were
// uploaded (#16: on Nvidia the water's shader sees other values than the texture holds in memory)
void VulkanBackend::BumpCheck(Tex& tx, const char* when) {
    static int n;
    if (n >= 12 || tx.check.empty() || !tx.img.image) return;
    n++;
    EndRendering();
    VkBuffer buf;
    VmaAllocation mem;
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = tx.check.size();
    bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo ai;
    if (vmaCreateBuffer(vma_, &bci, &aci, &buf, &mem, &ai) != VK_SUCCESS) return;
    VkImageLayout was = tx.img.layout;
    Barrier(tx.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy c{};
    c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    c.imageExtent = {tx.img.w, tx.img.h, 1};
    d_.vkCmdCopyImageToBuffer(cmd_, tx.img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &c);
    Barrier(tx.img, was);
    Submit();
    vmaInvalidateAllocation(vma_, mem, 0, VK_WHOLE_SIZE);
    const uint8_t* gpu = static_cast<const uint8_t*>(ai.pMappedData);
    const uint8_t* cpu = tx.check.data();
    size_t texels = tx.check.size() / 4, differ = 0;
    int umin = 127, umax = -128, vmin = 127, vmax = -128;
    for (size_t i = 0; i < texels; i++) {
        if (gpu[i * 4] != cpu[i * 4] || gpu[i * 4 + 1] != cpu[i * 4 + 1]) differ++;
        umin = std::min(umin, int(int8_t(gpu[i * 4]))); umax = std::max(umax, int(int8_t(gpu[i * 4])));
        vmin = std::min(vmin, int(int8_t(gpu[i * 4 + 1]))); vmax = std::max(vmax, int(int8_t(gpu[i * 4 + 1])));
    }
    VkMemoryPropertyFlags imf = 0, rf = 0;
    vmaGetAllocationMemoryProperties(vma_, tx.img.mem, &imf);
    vmaGetAllocationMemoryProperties(vma_, ring_mem_, &rf);
    std::fprintf(stderr,
                 "[D3D] bump map %s: the GPU's copy %s the upload (%zu of %zu texels differ; GPU u %d..%d v %d..%d; "
                 "first bytes GPU %02X%02X %02X%02X CPU %02X%02X %02X%02X; image %ux%u levels %u memory %X, ring memory %X)\n",
                 when, differ ? "DIFFERS from" : "matches", differ, texels, umin, umax, vmin, vmax, gpu[0], gpu[1], gpu[4], gpu[5],
                 cpu[0], cpu[1], cpu[4], cpu[5], tx.img.w, tx.img.h, tx.img.levels, imf, rf);
    vmaDestroyBuffer(vma_, buf, mem);
}

VkSampler VulkanBackend::Sampler(const State& s, int stage, const TexInfo& info) {
    const auto& t = s.tss[size_t(stage)];
    // D3DTEXF: 1 point, 2 linear, 3 anisotropic, 4 quincunx, 5 gaussian
    uint32_t mag = t[kTssMagFilter], min = t[kTssMinFilter], mip = t[kTssMipFilter];
    uint32_t au = t[kTssAddressU], av = t[kTssAddressV], aw = t[kTssAddressW], border = t[kTssBorderColor];
    // the console tests texel OP reference; Vulkan tests reference OP texel, so the sides swap
    static const uint8_t swap[8] = {0, 4, 2, 6, 1, 5, 3, 7};
    uint32_t cmp = info.shadow ? swap[uint32_t(Cmp(s.rs[kRsShadowFunc]))] : 0;
    // anisotropic filtering (a quality setting): linear with anisotropy, trilinear between levels
    bool smooth = aniso_level_ > 0 && !info.shadow;
    if (smooth) {
        mag = 2;
        min = 3;
        if (mip == 1) mip = 2;
    }
    uint64_t key = uint64_t(mag) | uint64_t(min) << 4 | uint64_t(mip) << 8 | uint64_t(au) << 12 | uint64_t(av) << 16 |
                   uint64_t(aw) << 20 | uint64_t(info.shadow ? 1 : 0) << 24 | uint64_t(border != 0) << 25 |
                   uint64_t(cmp) << 26 | uint64_t(t[kTssMaxMipLevel] & 15) << 30 | uint64_t(t[kTssMipLodBias]) << 34 |
                   uint64_t(smooth ? aniso_level_ : 0) << 50;
    auto it = samplers_.find(key);
    if (it != samplers_.end()) return it->second;
    VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    ci.magFilter = mag == 1 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    ci.minFilter = min == 1 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    ci.mipmapMode = mip == 2 ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    ci.addressModeU = Address(au);
    ci.addressModeV = Address(av);
    ci.addressModeW = Address(aw);
    ci.maxLod = mip ? VK_LOD_CLAMP_NONE : 0.25f;
    static bool nomip = std::getenv("NGB_D3D_NOMIP") != nullptr;   // debugging: the top level only
    if (nomip) ci.maxLod = 0.25f;
    ci.borderColor = border ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    if (min == 3 && anisotropy_) {
        ci.anisotropyEnable = VK_TRUE;
        ci.maxAnisotropy = smooth ? float(aniso_level_) : 8.0f;
    }
    {
        ci.mipLodBias = std::clamp(LodBias(t[kTssMipLodBias]), -lod_bias_max_, lod_bias_max_);
        static const bool biaslog = std::getenv("NGB_D3D_BIASLOG") != nullptr;   // debugging: the biases used, once each
        static std::set<uint32_t> told;
        if (biaslog && told.size() < 64 && told.insert(t[kTssMipLodBias]).second)
            std::fprintf(stderr, "[D3D] lod bias %08X (%g) -> %g\n", t[kTssMipLodBias], Bits(t[kTssMipLodBias]), ci.mipLodBias);
    }
    ci.minLod = float(t[kTssMaxMipLevel]);
    if (!mip) ci.maxLod = ci.minLod + 0.25f;
    if (info.shadow) {
        ci.compareEnable = VK_TRUE;
        ci.compareOp = VkCompareOp(cmp);
        static const char* shmode = std::getenv("NGB_D3D_SHADOW_MODE");   // #50 test: nearest instead of linear
        if (shmode && !std::strcmp(shmode, "nearest")) ci.magFilter = ci.minFilter = VK_FILTER_NEAREST;
        ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        ci.anisotropyEnable = VK_FALSE;
    }
    VkSampler smp = VK_NULL_HANDLE;
    d_.vkCreateSampler(dev_, &ci, nullptr, &smp);
    samplers_[key] = smp;
    return smp;
}

VkShaderModule VulkanBackend::Module(uint64_t key, const std::function<std::string()>& src, bool vertex) {
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        auto it = modules_.find(key);
        if (it != modules_.end()) return it->second;
    }
    std::string hlsl = src();
    // Left to the compiler thread only when it is new: a module whose SPIR-V an earlier session kept is
    // made at once (no glslang, just vkCreateShaderModule). Deferred, the draw was left out until the
    // prewarm thread got to it -- walls and effects flickering for a while after every launch (#21).
    if (Defer() && !SpirvKnown(hlsl, vertex)) {
        std::lock_guard<std::mutex> g(job_mu_);
        if (!pending_mods_.count(key) && std::getenv("NGB_D3D_HITCHLOG")) {   // NGB_D3D_HITCHLOG: what is left to the compiler thread
            static int n;
            if (n++ < 40) std::fprintf(stderr, "[DEFER] module %016llX %s (%zu chars)\n", (unsigned long long)key, vertex ? "vs" : "ps", hlsl.size());
        }
        if (!pending_mods_.count(key)) {
            pending_mods_[key] = true;
            jobs_.push_back({false, key, std::move(hlsl), vertex, PipeKey{}});
            job_cv_.notify_one();
        }
        return VK_NULL_HANDLE;
    }
    auto t0 = std::chrono::steady_clock::now();
    std::vector<uint32_t> spirv;
    VkShaderModule m = Compile(hlsl, vertex, spirv) ? MakeModule(spirv) : VK_NULL_HANDLE;
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        auto it = modules_.find(key);
        if (it != modules_.end()) {          // made meanwhile by the prewarm thread
            if (m) d_.vkDestroyShaderModule(dev_, m, nullptr);
            return it->second;
        }
        modules_[key] = m;
    }
    compile_us_ += uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count());
    compiles_++;
    return m;
}

// HLSL to SPIR-V, from the cache (by the hash of the HLSL) when it has it
bool VulkanBackend::SpirvKnown(const std::string& hlsl, bool vertex) {
    uint64_t h = Hash(hlsl.data(), hlsl.size(), vertex ? 0x5653ull : 0x5053ull);
    std::lock_guard<std::recursive_mutex> g(cache_mu_);
    return spirv_by_src_.count(h) != 0;
}

bool VulkanBackend::Compile(const std::string& hlsl, bool vertex, std::vector<uint32_t>& spirv) {
    uint64_t h = Hash(hlsl.data(), hlsl.size(), vertex ? 0x5653ull : 0x5053ull);
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        auto it = spirv_by_src_.find(h);
        if (it != spirv_by_src_.end()) {
            spirv = it->second;
            return true;
        }
    }
    std::string log;
    bool ok;
    {
        std::lock_guard<std::mutex> g(compile_mu_);
        ok = vkr::Compile(hlsl, vertex, spirv, &log);
    }
    if (!ok) {
        static int n;
        if (n++ < 8) std::fprintf(stderr, "[D3D] shader failed:\n%s\n--- source ---\n%s\n", log.c_str(), vkr::VulkanHlsl(hlsl).c_str());
        return false;
    }
    std::lock_guard<std::recursive_mutex> g(cache_mu_);
    spirv_by_src_[h] = spirv;
    if (spirv_) {
        uint32_t n = uint32_t(spirv.size());
        std::fwrite(&h, 8, 1, spirv_);
        std::fwrite(&n, 4, 1, spirv_);
        std::fwrite(spirv.data(), 4, n, spirv_);
        std::fflush(spirv_);
    }
    return true;
}

VkShaderModule VulkanBackend::MakeModule(const std::vector<uint32_t>& spirv) {
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = spirv.size() * 4;
    ci.pCode = spirv.data();
    VkShaderModule m = VK_NULL_HANDLE;
    d_.vkCreateShaderModule(dev_, &ci, nullptr, &m);
    return m;
}

VkPipeline VulkanBackend::Pipeline(const PipeKey& k) {
    VkShaderModule vs, ps;
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        auto it = pipelines_.find(k);
        if (it != pipelines_.end()) return it->second;
        auto a = modules_.find(k.vs), b = modules_.find(k.ps);
        vs = a != modules_.end() ? a->second : VK_NULL_HANDLE;
        ps = b != modules_.end() ? b->second : VK_NULL_HANDLE;
    }
    if (!vs || !ps) return VK_NULL_HANDLE;
    bool known;
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        known = known_pipes_.count(k) != 0;
    }
    // The same for a pipeline: one the seed or an earlier session made is made at once (the driver's
    // cache has it); a new one too while the frame's budget lasts, past it the compiler thread's, its draw
    // left out meanwhile.
    if (Defer() && !known) {
        std::lock_guard<std::mutex> g(job_mu_);
        if (!pending_pipes_.count(k) && std::getenv("NGB_D3D_HITCHLOG")) {   // NGB_D3D_HITCHLOG: what is left to the compiler thread
            static int n;
            if (n++ < 40) std::fprintf(stderr, "[DEFER] pipeline vs %016llX ps %016llX\n", (unsigned long long)k.vs, (unsigned long long)k.ps);
        }
        if (!pending_pipes_.count(k)) {
            pending_pipes_[k] = true;
            jobs_.push_back({true, 0, {}, false, k});
            job_cv_.notify_one();
        }
        return VK_NULL_HANDLE;
    }
    auto t0 = std::chrono::steady_clock::now();
    VkPipeline pipe = MakePipeline(k, vs, ps);
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        auto it = pipelines_.find(k);
        if (it != pipelines_.end()) {
            if (pipe) d_.vkDestroyPipeline(dev_, pipe, nullptr);
            return it->second;
        }
        pipelines_[k] = pipe;
    }
    if (pipe) {
        CacheRecord('K', 0, &k, sizeof(k));
        pipes_since_save_++;
    }
    compile_us_ += uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count());
    pipes_made_++;
    return pipe;
}

// Lines, or triangles drawn as their edges: the pipelines with a line width.
static bool Lines(const PipeKey& k) { return k.topo == 1 || k.fill == 0x1B01; }

VkPipeline VulkanBackend::MakePipeline(const PipeKey& k, VkShaderModule vs, VkShaderModule ps) {
    VkPipeline pipe = VK_NULL_HANDLE;
    if (vs && ps) {
        VkPipelineShaderStageCreateInfo st[2] = {{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
                                                 {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
        st[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        st[0].module = vs;
        st[0].pName = "main";
        st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        st[1].module = ps;
        st[1].pName = "main";
        VkVertexInputBindingDescription vb[16];
        VkVertexInputAttributeDescription va[16];
        uint32_t nb = 0, na = 0;
        bool used[16] = {};
        for (uint32_t r = 0; r < 16; r++) {
            if (!((k.regs >> r) & 1)) continue;
            uint32_t e = k.elements[r];
            uint32_t buf = (e >> 8) & 0xFF;
            va[na++] = {r, buf, VertexFormat(uint8_t(e & 0xFF)), e >> 16};
            used[buf] = true;
        }
        for (uint32_t b = 0; b < 16; b++)
            if (used[b]) vb[nb++] = {b, k.strides[b], VK_VERTEX_INPUT_RATE_VERTEX};
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vi.vertexBindingDescriptionCount = nb;
        vi.pVertexBindingDescriptions = vb;
        vi.vertexAttributeDescriptionCount = na;
        vi.pVertexAttributeDescriptions = va;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = k.topo == 0 ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST
                      : k.topo == 1 ? (k.strip ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                      : (k.strip ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount = 1;
        vp.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode = k.fill == 0x1B01 ? VK_POLYGON_MODE_LINE : k.fill == 0x1B00 ? VK_POLYGON_MODE_POINT : VK_POLYGON_MODE_FILL;
        rs.cullMode = !k.cull ? VK_CULL_MODE_NONE : k.cullfront ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT;
        // Direct3D's winding is in its y-down screen space, Vulkan's in the framebuffer after the
        // viewport transform: with the flipped viewport the two agree, so no inversion
        rs.frontFace = k.ccw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        rs.depthBiasEnable = k.zbias || k.zslope || (k.has_ds & kDsDynamicBias);
        rs.depthBiasConstantFactor = float(k.zbias);
        rs.depthBiasSlopeFactor = k.zslope;
        rs.lineWidth = 1.0f;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VkSampleCountFlagBits(k.samples ? k.samples : 1);
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        ds.depthTestEnable = k.zen;
        ds.depthWriteEnable = k.zen && k.zwrite;
        ds.depthCompareOp = k.zen ? Cmp(k.zfunc) : VK_COMPARE_OP_ALWAYS;
        ds.stencilTestEnable = k.sten;
        VkStencilOpState so{StencilOp(k.sfail), StencilOp(k.spass), StencilOp(k.szfail), Cmp(k.sfunc), k.smask, k.swmask, 0};
        ds.front = ds.back = so;
        VkPipelineColorBlendAttachmentState cb{};
        cb.blendEnable = k.blend;
        cb.srcColorBlendFactor = Blend(k.src, false);
        cb.dstColorBlendFactor = Blend(k.dst, false);
        cb.colorBlendOp = BlendOp(k.op);
        cb.srcAlphaBlendFactor = Blend(k.src, true);
        cb.dstAlphaBlendFactor = Blend(k.dst, true);
        cb.alphaBlendOp = BlendOp(k.op);
        cb.colorWriteMask = k.wmask;
        VkPipelineColorBlendStateCreateInfo cbs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cbs.attachmentCount = k.has_rt ? 1 : 0;
        cbs.pAttachments = &cb;
        // the line width only on the line pipelines: the others, nearly all, stay as the caches have them
        VkDynamicState dyn[6] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_BLEND_CONSTANTS,
                                 VK_DYNAMIC_STATE_STENCIL_REFERENCE};
        VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dy.dynamicStateCount = 4;
        if (Lines(k)) dyn[dy.dynamicStateCount++] = VK_DYNAMIC_STATE_LINE_WIDTH;
        if (k.has_ds & kDsDynamicBias) dyn[dy.dynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
        dy.pDynamicStates = dyn;
        VkFormat cf = kColorFormat;
        VkPipelineRenderingCreateInfo pr{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        pr.colorAttachmentCount = k.has_rt ? 1 : 0;
        pr.pColorAttachmentFormats = &cf;
        pr.depthAttachmentFormat = k.has_ds ? ds_format_ : VK_FORMAT_UNDEFINED;
        pr.stencilAttachmentFormat = k.has_ds ? ds_format_ : VK_FORMAT_UNDEFINED;
        VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        ci.pNext = &pr;
        ci.stageCount = 2;
        ci.pStages = st;
        ci.pVertexInputState = &vi;
        ci.pInputAssemblyState = &ia;
        ci.pViewportState = &vp;
        ci.pRasterizationState = &rs;
        ci.pMultisampleState = &ms;
        ci.pDepthStencilState = &ds;
        ci.pColorBlendState = &cbs;
        ci.pDynamicState = &dy;
        ci.layout = pipe_layout_;
        if (d_.vkCreateGraphicsPipelines(dev_, pcache_, 1, &ci, nullptr, &pipe) != VK_SUCCESS) pipe = VK_NULL_HANDLE;
    }
    return pipe;
}

// D3DLIGHT8 as floats: Type, Diffuse[4], Specular[4], Ambient[4], Position[3], Direction[3], Range,
// Falloff, Attenuation0 to 2, Theta, Phi; to view space for the fixed function shader
void LightToView(const std::array<uint32_t, 26>& l, const float* vm, float (&o)[7][4]) {
    float f[26];
    std::memcpy(f, l.data(), sizeof(f));
    for (int j = 0; j < 4; j++) {
        o[0][j] = f[1 + j];
        o[1][j] = f[5 + j];
        o[2][j] = f[9 + j];
    }
    const float* pos = f + 13;
    const float* dir = f + 16;
    for (int j = 0; j < 3; j++) {
        o[3][j] = pos[0] * vm[j] + pos[1] * vm[4 + j] + pos[2] * vm[8 + j] + vm[12 + j];
        o[4][j] = dir[0] * vm[j] + dir[1] * vm[4 + j] + dir[2] * vm[8 + j];
    }
    o[3][3] = f[19];
    o[4][3] = f[20];
    o[5][0] = f[21];
    o[5][1] = f[22];
    o[5][2] = f[23];
    o[6][0] = std::cos(f[24] * 0.5f);
    o[6][1] = std::cos(f[25] * 0.5f);
}

void InverseTranspose3x3(const float* wv, float* n) {
    float det = wv[0] * (wv[5] * wv[10] - wv[6] * wv[9]) - wv[1] * (wv[4] * wv[10] - wv[6] * wv[8]) +
                wv[2] * (wv[4] * wv[9] - wv[5] * wv[8]);
    float id = det != 0 ? 1.0f / det : 0.0f;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) {
            int r1 = (r + 1) % 3, r2 = (r + 2) % 3, c1 = (c + 1) % 3, c2 = (c + 2) % 3;
            n[r * 4 + c] = (wv[r1 * 4 + c1] * wv[r2 * 4 + c2] - wv[r1 * 4 + c2] * wv[r2 * 4 + c1]) * id;
        }
}

void MatMul(const float* x, const float* y, float* o) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            o[r * 4 + c] = x[r * 4] * y[c] + x[r * 4 + 1] * y[4 + c] + x[r * 4 + 2] * y[8 + c] + x[r * 4 + 3] * y[12 + c];
}
void Transpose(const float* m, float* o) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) o[c * 4 + r] = m[r * 4 + c];
}

#include "spark_aspect.h"

void VulkanBackend::CapturePeek(const State& s, Peek& p) {
    if (towers_frame_ != peek_frames_) {   // the chunk table once a frame (it changes as an area loads)
        towers_frame_ = peek_frames_;
        TowersRange(towers_lo_, towers_hi_);
    }
    p.draw_node = g_draw_node;
    p.chunk_slot = int32_t(DrawChunkSlot());
    uint32_t vb = s.streams[0].vb;
    p.towers = vb && vb >= towers_lo_ && vb < towers_hi_;
    p.stretched = uint8_t(ngb_2d_stretched());
    p.screen = uint8_t(ngb_2d_screen_flags());
    p.hud_shape = ngb_hud_shape();
}

void VulkanBackend::CaptureQuick(QuickPeek& q) {
    peek_frames_++;
    q.percent = ngb_pc_setting(20);
    q.frozen = 0;
    q.layer.clear();
    if (!q.percent) return;
    q.layer.resize(size_t(NGB_QUICK_WIDTH) * NGB_QUICK_HEIGHT);
    if (!ngb_quick_layer(q.layer.data())) {   // (its input steps here, once a frame)
        q.layer.clear();
        return;
    }
    q.frozen = ngb_quick_frozen();
}

void VulkanBackend::Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) {
    if (!pk_given_) CapturePeek(s, pk_);
    st_.draws++;
    if (!Assemble(s, prim, start, count, indices, asm_)) { st_.noasm++; return; }
    ClassifyGlowTarget(s,asm_,count);
    Target* rt = EnsureTarget(s.rt, false);
    Target* ds = s.zs.header && s.rs[kRsZEnable] | s.rs[kRsStencilEnable] ? EnsureTarget(s.zs, true) : nullptr;
    if (rt && ds && (rt->img.w != ds->img.w || rt->img.h != ds->img.h || rt->samples != ds->samples))
        ds = nullptr;                                  // as BeginRendering has it
    if (!rt && !ds) { st_.notarget++; return; }
    if (ContinuousGlowEnabled()) InvalidateContinuousGlow(s.rt.data);
    SparkEndpoint spark_ends[2]{};
    SparkQuadVertex spark_vertices[6]{};
    Assembled spark_assembled;
    bool spark_quad=false;
    if (rt && rt->gw>=640 && rt->gh>=448 && SparkFixEnabled() && SparkAspect()>16.0/9.0 &&
        SparkEndpoints(s,prim,count,asm_,spark_ends)) {
        // Match the original hardware line-width clamp at this actual target scale.
        float scale=float(rt->img.w)/float(rt->gw);
        float width=SparkThickness()*(wide_lines_?std::clamp(Bits(s.rs[kRsLineWidth])*scale,1.0f,line_max_)/scale:1.0f/scale);
        spark_quad=ExpandSpark(s,spark_ends,float(SparkAspect()/(16.0/9.0)),width,spark_vertices,spark_assembled);
    }
    Assembled& a=spark_quad?spark_assembled:asm_;
    rt_draws_[s.rt.data]++;

    // vertex shader: the title's program, or fixed function for its declaration
    bool prog = (s.vs_handle & 1) && s.vs && !s.vs->program.empty();
    bool quad = count <= kQuadVertices && !s.rs[kRsZEnable];
    FixedKey fk;
    if (!prog) fk = MakeFixedKey(s);
    uint64_t vkey;
    if (prog) {
        if (!s.vs->hash) s.vs->hash = Hash(s.vs->program.data(), s.vs->program.size() * 4) | 1;
        vkey = Hash(&a.decode, 8, s.vs->hash ^ a.regs);
    } else {
        vkey = Hash(&a.decode, 8, Hash(&s.vs_handle, 4, a.regs | 0x10000));
        if (a.pretransformed) vkey ^= 0x5A5A;
        vkey = Hash(&fk, sizeof(fk), vkey);
    }
    if (quad) vkey ^= 0xC3C30000;
    // a point list: the shader says each point's size (PointHlsl), and a sprite's pixel shader makes its
    // coordinates (SpriteHlsl); made as drawn, not kept in the cache, whose recipes make the plain ones
    bool points = a.topology == Topology::Points;
    bool sprite = points && s.rs[kRsPointSpriteEnable];
    if (points) vkey ^= 0x9D7E0000;
    if (spark_quad) vkey ^= 0x53A4B197E42D061Bull;
    static const bool spritelog = std::getenv("NGB_D3D_SPRITELOG") != nullptr;
    if (sprite && spritelog) {   // NGB_D3D_SPRITELOG: each point sprite setup once, every stage (#51)
        static std::set<uint64_t> told;
        uint64_t k = uint64_t(s.textures[0]) << 32 ^ s.textures[3] ^ uint64_t(s.tss[0][kTssColorOp]) << 8;
        if (told.size() < 24 && told.insert(k).second) {
            std::fprintf(stderr, "[D3D] point sprites:");
            for (int st = 0; st < 4; st++) {
                TextureDesc td;
                bool h = s.textures[st] && Describe(s.textures[st], td);
                std::fprintf(stderr, " stage %d tex %08X @%08X fmt %X %ux%u levels %u op %u/%u lodbias %08X maxmip %u texcoord %X;", st,
                             s.textures[st], h ? td.data : 0, h ? td.format : 0, h ? td.width : 0, h ? td.height : 0,
                             h ? td.levels : 0, s.tss[st][kTssColorOp], s.tss[st][kTssAlphaOp], s.tss[st][kTssMipLodBias],
                             s.tss[st][kTssMaxMipLevel], s.tss[st][kTssTexCoordIndex]);
            }
            std::fprintf(stderr, " size %g min %g max %g\n", Bits(s.rs[kRsPointSize]), Bits(s.rs[kRsPointSizeMin]),
                         Bits(s.rs[kRsPointSizeMax]));
        }
    }
    bool vnew;
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        vnew = !modules_.count(vkey);
    }
    if (vnew && !points && !spark_quad) {   // how it is made, for the cache (ShaderCache)
        std::vector<uint8_t> r;
        if (prog) {
            CurrentVertexProgramRecipe(s.vs->program, a.regs, a.decode, quad, r);
            CacheRecord('V', vkey, r.data(), r.size());
        } else {
            uint8_t b[2 + 1 + 8 + sizeof(FixedKey)];
            std::memcpy(b, &a.regs, 2);
            b[2] = uint8_t(a.pretransformed);
            std::memcpy(b + 3, &a.decode, 8);
            std::memcpy(b + 11, &fk, sizeof(fk));
            CacheRecord('F', vkey, b, sizeof(b));
        }
    }
    async_now_ = async_;
    VkShaderModule vmod = Module(vkey, [&] {
        std::string src = prog ? VertexProgramHlsl(s.vs->program, a.regs, a.decode)
                               : FixedVertexHlsl(a.regs, a.pretransformed, a.decode, fk);
        if (spark_quad) src = SparkStrokeHlsl(std::move(src));
        // the quad step for a screen quad -- and for every pretransformed fixed-function draw, as the shader cache
        // rebuilds those (its record has no quad flag; the step does nothing unless QuadSnap turns it on)
        if (quad || (!prog && a.pretransformed)) src = QuadHlsl(std::move(src));
        if (points) src = PointHlsl(std::move(src), prog ? 0 : a.pretransformed ? 2 : 1);
        static const char* dump = std::getenv("NGB_D3D_SHADERDUMP");
        if (dump) {
            char path[512];
            std::snprintf(path, sizeof(path), "%s/vs_%016llx.hlsl", dump, (unsigned long long)vkey);
            if (FILE* f = std::fopen(path, "w")) { std::fputs(src.c_str(), f); std::fclose(f); }
        }
        return src;
    }, true);

    // textures, then the pixel shader made for them
    TexInfo info[4];
    VkImageView views[4];
    bool from_target[4];
    draw_rt_ = rt;
    for (int i = 0; i < 4; i++) views[i] = BindTexture(s, i, info[i], from_target[i]);
    draw_rt_ = nullptr;
    for (int i = 0; i < 4 && quad; i++)   // a screen quad's linearly filtered textures (QuadSnap's alpha weighting)
        if (info[i].kind == kTex2D && !info[i].shadow && (s.tss[size_t(i)][kTssMagFilter] >= 2 || aniso_level_ > 0))
            info[i].flags |= kTexLinear;
    static const bool bumplog = std::getenv("NGB_D3D_BUMPLOG") != nullptr;
    if ((info[0].flags & kTexSigned) && bumplog) {   // the water's stage 0 sampling (#16)
        static int n;
        if (n++ < 6) {
            const auto& t = s.tss[0];
            std::fprintf(stderr, "[D3D] water stage 0: mag %u min %u mip %u maxmip %u lodbias %08X addr %u %u\n", t[kTssMagFilter],
                         t[kTssMinFilter], t[kTssMipFilter], t[kTssMaxMipLevel], t[kTssMipLodBias], t[kTssAddressU], t[kTssAddressV]);
        }
    }
    if (bumpcheck_ && (info[0].flags & kTexSigned) && bump_key_) {   // the water's ripple map, as the shader is about to read it
        auto bit = textures_.find(bump_key_);
        if (bit != textures_.end()) {
            if (bit->second.img.view != views[0])
                std::fprintf(stderr, "[D3D] water draw: tex0 is not the bump map last bound\n");
            BumpCheck(bit->second, "at the water draw");
        }
    }
    static bool noshadow = std::getenv("NGB_D3D_NOSHADOW") != nullptr;   // debugging: no shadow-map passes
    if (noshadow && (info[0].shadow || info[1].shadow || info[2].shadow || info[3].shadow)) return;
    PixelKey pk = MakePixelKey(s, info);
    pk.quad = quad;
    if (sprite && !pk.programmable) {   // stage 3 alone, whatever the stages before it hold (PixelHlsl)
        const auto& t = s.tss[3];
        uint32_t v[9] = {t[kTssColorOp], t[kTssColorArg1], t[kTssColorArg2], t[kTssColorArg0], t[kTssAlphaOp],
                         t[kTssAlphaArg1], t[kTssAlphaArg2], t[kTssAlphaArg0], t[kTssResultArg]};
        std::memset(pk.ff, 0, sizeof(pk.ff));
        std::memcpy(pk.ff[3], v, sizeof(v));
        pk.alphakill &= 8;
        pk.sprite = 1;
    }
    uint64_t pkey = Hash(&pk, sizeof(pk)) ^ 0x9E3779B97F4A7C15ull;
    if (IdView()) pkey ^= 0x1D1D1D1D1D1D1D1Dull * uint64_t(IdViewMode());   // other shaders: other modules in the cache
    if (sprite) pkey ^= 0x51D5;
    // debugging: NGB_D3D_PSDEBUG=flat (solid red) | tex (stage 0's texture) | uv | diffuse
    static const char* psdbg = std::getenv("NGB_D3D_PSDEBUG");
    static int flat = psdbg && !std::strcmp(psdbg, "flat");
    if (psdbg) pkey = Hash(psdbg, std::strlen(psdbg), pkey);
    if (!psdbg && !sprite) {
        bool pnew;
        {
            std::lock_guard<std::recursive_mutex> g(cache_mu_);
            pnew = !modules_.count(pkey);
        }
        if (pnew) CacheRecord('P', pkey, &pk, sizeof(pk));
    }
    VkShaderModule pmod = Module(pkey, [&] {
        std::string src = PixelHlsl(pk);
        if (sprite) src = SpriteHlsl(std::move(src));
        // NGB_D3D_PSDEBUG=bump | duv | copy: the water's draws only (a bump map in stage 0 displacing stage
        // 2's texture, #16): the ripple texture's raw bytes as sampled (u red, v green: two-tone, as small
        // signed values look unsigned), the displacement decoded from it (grey = none), or stage 2's
        // texture undisplaced (the copy of the screen the water refracts)
        bool water_dbg = psdbg && (!std::strcmp(psdbg, "bump") || !std::strcmp(psdbg, "duv") || !std::strcmp(psdbg, "copy") ||
                                   !std::strcmp(psdbg, "lod") || !std::strcmp(psdbg, "texel"));
        bool water_draw = (pk.tex[0].flags & kTexSigned) && pk.tex[2].kind;
        // NGB_D3D_PSDEBUG=spr: point sprites only, their texture coordinates as red and green (#51)
        bool spr_dbg = psdbg && !std::strcmp(psdbg, "spr");
        // NGB_D3D_PSDEBUG=sprtex: point sprites only, stage 3's texture (the sprite's picture) at its top level
        bool sprtex_dbg = psdbg && !std::strcmp(psdbg, "sprtex");
        if (spr_dbg && sprite) {
            size_t at = src.find("float4 main(");
            src = src.substr(0, at) + "float4 main(VOut i) : SV_Target { float2 s = (i.pos.xy - i.ptc.xy) / i.ptc.z + 0.5; "
                  "return float4(saturate(s), s.x < 0 || s.y < 0 || s.x > 1 || s.y > 1 ? 1 : 0, 1); }\n";
        } else if (sprtex_dbg && sprite) {
            size_t at = src.find("float4 main(");
            src = src.substr(0, at) + "float4 main(VOut i) : SV_Target { float2 s = (i.pos.xy - i.ptc.xy) / i.ptc.z + 0.5; "
                  "float4 t = tex3.SampleLevel(smp3, s, 0); return float4(t.rgb * t.a, 1); }\n";
        } else if (psdbg && !spr_dbg && !sprtex_dbg && (!water_dbg || water_draw)) {
            size_t at = src.find("float4 main(");
            std::string body = flat ? "return float4(1, 0, 0, 1);"
                : !std::strcmp(psdbg, "bump") ? "float4 t0 = tex0.Sample(smp0, i.tc0.xy / i.tc0.w); return float4(t0.b, t0.g, 0, 1);"
                : !std::strcmp(psdbg, "duv") ? "float2 d = bumpduv0(i.tc0.xy / i.tc0.w); return float4(d * 0.5 + 0.5, 0, 1);"
                : !std::strcmp(psdbg, "copy") ? "return float4(tex2.Sample(smp2, i.tc2.xy + texsize[2].zw).rgb, 1);"
                // the ripple texture as the GPU reports it: its level of detail here / 4 red, levels / 4 green,
                // width / 256 blue (on AMD: green 0.75, blue 0.5) -- and its texels one by one, decoded at the
                // top level without filtering, 8 times as strong (grey none) (#16)
                : !std::strcmp(psdbg, "lod") ? "uint w, h, n; tex0.GetDimensions(0, w, h, n);"
                                               " float l = tex0.CalculateLevelOfDetail(smp0, i.tc0.xy / i.tc0.w);"
                                               " return float4(saturate(l / 4), n / 4.0, w / 256.0, 1);"
                : !std::strcmp(psdbg, "texel") ? "uint w, h, n; tex0.GetDimensions(0, w, h, n); int2 dim = int2(w, h);"
                                                 " int2 p = int2(floor(frac(i.tc0.xy / i.tc0.w) * float2(dim)));"
                                                 " float2 d = bumptexel0(p, dim, 0); return float4(d * 8 + 0.5, 0.5, 1);"
                : !std::strcmp(psdbg, "tex") ? "return float4(tex0.Sample(smp0, i.tc0.xy).rgb, 1);"
                : !std::strcmp(psdbg, "uv") ? "return float4(frac(i.tc0.xy), 0, 1);"
                : !std::strcmp(psdbg, "shadow") ? (pk.tex[0].shadow
                    ? "float2 uv = (i.tc0.xy / i.tc0.w) / texsize[0].xy + texsize[0].zw;"
                      " float t = shadow0(uv, i.tc0.z / i.tc0.w, 1, i.pos.xy, texsize[0].xy).x;"
                      " return float4(i.d0.rgb * t, 1);"
                    : "return float4(i.d0.rgb, 1);")
                : "return float4(i.d0.rgb, 1);";
            src = src.substr(0, at) + "float4 main(VOut i) : SV_Target { " + body + " }\n";
        }
        static const char* dump = std::getenv("NGB_D3D_SHADERDUMP");
        if (dump) {
            char path[512];
            std::snprintf(path, sizeof(path), "%s/ps_%016llx.hlsl", dump, (unsigned long long)pkey);
            if (FILE* f = std::fopen(path, "w")) { std::fputs(src.c_str(), f); std::fclose(f); }
        }
        return src;
    }, false);

    // the pipeline
    PipeKey k;
    int32_t pull_steps = 0;   // the map pieces' pull: depth steps and slope added to the game's own bias
    float pull_slope = 0.0f;
    std::memset(&k, 0, sizeof(k));
    k.vs = vkey;
    k.ps = pkey;
    k.regs = a.regs;
    for (int r = 0; r < 16; r++)
        if ((a.regs >> r) & 1) {
            const VertexElement& e = a.elements[size_t(r)];
            k.elements[r] = e.format | uint32_t(e.buffer) << 8 | uint32_t(e.offset) << 16;
        }
    for (uint32_t b = 0; b < a.buffer_count; b++) k.strides[b] = a.buffers[b].stride;
    k.topo = uint32_t(a.topology);
    k.strip = a.strip;
    k.blend = SimpleState(s, kMBlendEnable) != 0;
    if (k.blend) {
        k.src = SimpleState(s, kMSrcBlend);
        k.dst = SimpleState(s, kMDestBlend);
        k.op = SimpleState(s, kMBlendOp);
    }
    uint32_t cw = SimpleState(s, kMColorWriteEnable);
    k.wmask = ((cw >> 16) & 1) | ((cw >> 8) & 1) << 1 | (cw & 1) << 2 | ((cw >> 24) & 1) << 3;
    k.has_rt = rt != nullptr;
    k.has_ds = ds != nullptr;
    k.samples = rt ? rt->samples : ds ? ds->samples : 1;
    if (ds) {
        k.zen = s.rs[kRsZEnable] != 0;
        if (k.zen) {
            k.zwrite = SimpleState(s, kMZWriteEnable) != 0;
            k.zfunc = SimpleState(s, kMZFunc);
        }
        k.sten = s.rs[kRsStencilEnable] != 0;
        if (k.sten) {
            k.sfunc = SimpleState(s, kMStencilFunc);
            k.smask = SimpleState(s, kMStencilMask) & 0xFF;
            k.swmask = SimpleState(s, kMStencilWriteMask) & 0xFF;
            k.sfail = s.rs[kRsStencilFail];
            k.szfail = SimpleState(s, kMStencilZFail);
            k.spass = SimpleState(s, kMStencilPass);
        }
        if (a.topology == Topology::Triangles && SimpleState(s, kMSolidOffsetEnable)) {
            k.zbias = int32_t(std::lround(Bits(SimpleState(s, kMDepthOffset)) * 16777215.0f / s.zscale));
            k.zslope = Bits(SimpleState(s, kMSlopeScale));
        }
        if (SectionPull() > 0 && a.topology == Topology::Triangles && k.zen && k.has_ds) {   // a pull of 0 turns it off
            // by the vertex buffer (its header in the towers' chunk), not the texture: every pass over the same mesh -- the
            // depth pre-pass, untextured, and Ryu's shadow laid over it -- then moves with it, or the shadow breaks up
            if (pk_.towers) {
                k.has_ds |= kDsDynamicBias;   // the pull set at the draw: any setting without a pipeline of its own
                {
                    // mostly by the surface's depth slope: the ties of two meshes in one plane come from rounding in
                    // where their vertices land on screen, so they grow with how steeply the depth changes across
                    // a pixel -- at the drawbridge within 0.15 of a console pixel's worth (6 to 600 steps a pixel
                    // there). Half a console pixel's worth covers them on an edge-on wall and is next to nothing on
                    // a surface facing the camera, where a constant pull brought things behind a facade through it;
                    // 2 depth steps more for the flattest ties.
                    float scale = rt ? float(rt->img.w) / float(std::max<uint32_t>(rt->gw, 1)) : 1.0f;
                    pull_slope = -SectionSlope() * std::max(scale, 1.0f);
                    pull_steps = -SectionPull();
                    section_pulled_++;
                }
            }
        }
        // Ryu's shadow laid over what is already drawn (a pass sampling a shadow map, depth tested, not written): the
        // same surface again by another vertex path, so its depth ties with the first pass's and the ties grow with
        // the depth slope in pixels -- from 4x up the shadow on a wall broke up and flickered as he hung from a
        // ledge (#106). Pulled as the towers' pieces are, by half a console pixel's worth of slope.
        static const bool shadow_pull = [] { const char* e = std::getenv("NGB_D3D_SHADOW_PULL"); return !e || *e != '0'; }();
        bool receiver = false;
        for (int i = 0; i < 4; i++) receiver |= from_target[i] && info[i].shadow != 0;
        if (shadow_pull && receiver && !(k.has_ds & kDsDynamicBias) && a.topology == Topology::Triangles && k.zen &&
            !k.zwrite && k.has_ds) {
            k.has_ds |= kDsDynamicBias;
            float scale = rt ? float(rt->img.w) / float(std::max<uint32_t>(rt->gw, 1)) : 1.0f;
            pull_slope = -SectionSlope() * std::max(scale, 1.0f);
            pull_steps = -SectionPull();
        }
    }
    // the console culls the CULLMODE winding (D3DCULL_CW 0x900, CCW 0x901); FRONTFACE says which
    // winding is the front, so the culled side is the front when the two agree
    uint32_t cull = s.rs[kRsCullMode], front = s.rs[kRsFrontFace];
    k.cull = cull != 0 && a.topology == Topology::Triangles;
    k.ccw = front == 0x901;              // front faces counter clockwise (in Direct3D's y-down screen)
    k.cullfront = k.cull && cull == front;
    k.fill = s.rs[kRsFillMode];
    if (spark_quad) { k.cull=0;k.fill=0x1B02;k.zbias=0;k.zslope=0;k.has_ds&=~kDsDynamicBias; }
    static bool nocull = std::getenv("NGB_D3D_NOCULL") != nullptr;    // debugging
    static bool nodepth = std::getenv("NGB_D3D_NODEPTH") != nullptr;  // debugging
    static bool cullinv = std::getenv("NGB_D3D_CULLINV") != nullptr;  // debugging: the other side
    if (nocull) k.cull = 0;
    if (cullinv) k.cullfront = !k.cullfront;
    if (nodepth) { k.zen = 0; }
    if (flat) { k.blend = 0; k.zen = 0; k.sten = 0; k.wmask = 0xF; k.cull = 0; }
    static int dbg_shadow = psdbg && !std::strcmp(psdbg, "shadow");
    if (dbg_shadow && pk.tex[0].shadow) { k.blend = 0; k.wmask = 0xF; }
    TextureDesc glow_desc{};
    bool glow_pass = ContinuousGlowEnabled() && rt && rt->gw >= 640 && rt->gh >= 448 && a.pretransformed && !prog && !s.ps && count == 4 &&
        from_target[0] && !info[0].shadow && k.blend && k.src == 1 && k.dst == 1 && k.op == 0x8006 &&
        !k.zen && k.wmask == 15 && s.tss[0][kTssColorOp] == 5 && s.tss[0][kTssColorArg1] == 2 &&
        s.tss[0][kTssColorArg2] == 3 && s.tss[1][kTssColorOp] == 1 &&
        Describe(s.textures[0],glow_desc) && glow_desc.width == 128 && glow_desc.height == 128;
    if (glow_pass) {
        const auto& element = a.elements[0];
        glow_pass = element.buffer < a.buffer_count;
        if (glow_pass) {
            const auto& buffer = a.buffers[element.buffer];
            glow_pass = buffer.src && buffer.stride && size_t(element.offset) + size_t(buffer.stride)*3 + 8 <= buffer.bytes;
            if (glow_pass) {
                float x0=1e10f,y0=1e10f,x1=-1e10f,y1=-1e10f;
                for (int i=0;i<4;++i) {
                    float xy[2]; std::memcpy(xy,buffer.src+size_t(i)*buffer.stride+element.offset,8);
                    x0=std::min(x0,xy[0]); y0=std::min(y0,xy[1]); x1=std::max(x1,xy[0]); y1=std::max(y1,xy[1]);
                }
                glow_pass = std::abs(x0) <= 0.5f && std::abs(y0) <= 0.5f &&
                    std::abs(x1-float(rt->gw)) <= 0.5f && std::abs(y1-float(rt->gh)) <= 0.5f;
            }
        }
    }
    bool continuous_composite=glow_pass &&
        continuous_record_[2].frame==presents_ && continuous_record_[2].data==(glow_desc.data&0x7FFFFFFFu);
    glow_pass=continuous_composite;
    if (glow_pass && GlowStrength()<1.0f) {
        async_now_=false;
        pkey^=0x674C4F5753540001ull;k.ps=pkey;
        pmod=Module(pkey,[&] {return GlowStrengthHlsl(PixelHlsl(pk));},false);
    }
    VkPipeline pipe = vmod && pmod ? Pipeline(k) : VK_NULL_HANDLE;
    if (!pipe && vmod && pmod && (k.has_ds & kDsDynamicBias)) {
        // the map piece's pipeline with the bias at the draw still being made: drawn meanwhile without the pull,
        // by the pipeline it had before (the caches have it), rather than left out
        PipeKey plain = k;
        plain.has_ds &= ~kDsDynamicBias;
        if ((pipe = Pipeline(plain))) k = plain;
    }
    async_now_ = false;
    if (!pipe) {
        st_.nopipe++;
        if (async_) skipped_++;
        return;
    }
    if (glow_pass && GlowSoftenMode()) {
        auto source=targets_.find(uint64_t(glow_desc.data&0x7FFFFFFFu)<<1);
        if (source==targets_.end() || !SoftenGlow(source->second.img))
            ngb_renderer_fatal("Could not create the glow softening pass.");
        views[0]=glow_soft_[1].view;
    }
    st_.drawn++;

    // constants
    uint32_t gw, gh;
    SurfaceSize(rt ? s.rt : s.zs, gw, gh);
    DrawConstants c{};
    c.surf[0] = float(gw);
    c.surf[1] = float(gh);
    c.surf[2] = 1.0f / s.zscale;
    c.surf[3] = float((rt ? rt->img : ds->img).w) / float(gw);   // the target's pixels to the console's
    if (s.rs[kRsFogEnable]) {
        c.fog[0] = Bits(s.rs[kRsFogStart]);
        c.fog[1] = Bits(s.rs[kRsFogEnd]);
        c.fog[2] = Bits(s.rs[kRsFogDensity]);
        c.fog[3] = float(s.rs[kRsFogTableMode]);
        if (s.rs[kRsFogTableMode] == 0 && prog) c.fog[3] = 4;   // the vertex program's oFog.x is the factor (fogfactor)
    }
    Color(s.rs[kRsFogColor], c.fog_color);
    c.misc[0] = float(SimpleState(s, kMAlphaRef) & 0xFF);
    c.misc[1] = SimpleState(s, kMAlphaTestEnable) ? float(int(Cmp(SimpleState(s, kMAlphaFunc)))) : 7.0f;
    c.misc[2] = float(int(Cmp(s.rs[kRsShadowFunc])));
    c.misc[3] = s.rs[kRsSpecularEnable] ? 1.0f : 0.0f;
    for (int i = 0; i < 8; i++) {
        Color(SimpleState(s, kMPsConstant0 + i * 4), c.pc[i * 2]);
        Color(SimpleState(s, kMPsConstant1 + i * 4), c.pc[i * 2 + 1]);
    }
    Color(SimpleState(s, kMPsFinalConstant0), c.pc[16]);
    Color(SimpleState(s, kMPsFinalConstant1), c.pc[17]);
    static const bool psclog = std::getenv("NGB_D3D_PSCLOG") != nullptr;
    if (s.ps && psclog) {   // debugging: each pixel shader's combiner constants, once (#29)
        static std::set<uint64_t> told;
        uint64_t k = uint64_t(s.ps->handle) << 32 ^ SimpleState(s, kMPsConstant0);
        if (told.size() < 64 && told.insert(k).second)
            std::fprintf(stderr, "[D3D] ps %08X c0 %08X %08X c1 %08X %08X final %08X %08X\n", s.ps->handle,
                         SimpleState(s, kMPsConstant0), SimpleState(s, kMPsConstant0 + 4), SimpleState(s, kMPsConstant1),
                         SimpleState(s, kMPsConstant1 + 4), SimpleState(s, kMPsFinalConstant0), SimpleState(s, kMPsFinalConstant1));
    }
    Color(s.rs[kRsTextureFactor], c.tfactor);
    for (int i = 0; i < 4; i++) {
        TextureDesc d;
        if (s.textures[size_t(i)] && Describe(s.textures[size_t(i)], d)) {
            c.texsize[i][0] = float(d.width);
            c.texsize[i][1] = float(d.height);
            // A depth target read as a shadow map: half a texel up and left. A colour target is read as drawn: the
            // game's copies of it (the pause menu's picture of the game behind it, 720x480 to 512x512 and back)
            // are drawn from -0.5 and land texel on pixel; half a texel more at each step moved the picture a
            // console pixel right and down as a menu opened (#99).
            static const bool rt_half = std::getenv("NGB_D3D_RT_HALF_TEXEL") != nullptr;   // the old way, to compare
            static const bool sh_half = [] { const char* e = std::getenv("NGB_D3D_SHADOW_HALF_TEXEL"); return !e || *e != '0'; }();
            if (from_target[i] && ((info[i].shadow && sh_half) || rt_half)) {
                c.texsize[i][2] = -0.5f / float(d.width);
                c.texsize[i][3] = -0.5f / float(d.height);
            }
        }
        const auto& t = s.tss[size_t(i)];
        c.bump[i][0] = Bits(t[kTssBumpEnvMat00]);
        c.bump[i][1] = Bits(t[kTssBumpEnvMat01]);
        c.bump[i][2] = Bits(t[kTssBumpEnvMat10]);
        c.bump[i][3] = Bits(t[kTssBumpEnvMat11]);
        c.bumpl[i][0] = Bits(t[kTssBumpEnvLScale]);
        c.bumpl[i][1] = Bits(t[kTssBumpEnvLOffset]);
        // for a bump map decoded in the shader (bumpduv): the level of detail the sampler would use, its
        // bias and most detailed level
        c.bumpl[i][2] = LodBias(t[kTssMipLodBias]);
        c.bumpl[i][3] = float(t[kTssMaxMipLevel]);
    }
    if (glow_pass) c.bump[3][2]=GlowStrength(); // unused by this single-texture draw
    // The 2D (screen-space draws into the back buffer) on a wider screen. The game's 2D screens -- menus,
    // the title, chapter cards (src/pc_menu.c says which) -- in its 16:9 look, stretched no wider than 16:9,
    // or with Menus 4:3 the console's 4:3 picture in the middle. In play its HUD and words in their own
    // shape (ngb_hud_shape: 4:3 over the screen's shape): the top of the screen's HUD keeps to its corners
    // and everything else is narrowed alike about the middle (words and subtitles are drawn in pieces);
    // what spans the screen (fades, letterbox bars) and what is off it as drawn.
    float hud_clip0 = -1, hud_clip1 = -1;   // a menu's 4:3 area (guest pixels), the draw clipped to it
    float hud_clipy0 = -1, hud_clipy1 = -1;   // (and its height, on a narrower screen)
    const PipeKey& pipe_k = k;   // (k below the screen's shape)
    // A pass over an effect target a pixel or two short of its edge: the heat haze over lava draws its
    // 512x256 picture from 0 to 510 and 254, and the columns and rows it leaves out (never cleared:
    // black here, white on the 360's emulator, harmless leftovers on the console) showed as a strip
    // down the right of the screen (#55). Such a quad is stretched to the target's edge.
    if (a.pretransformed && rt && rt->gw < 640 && count <= 4) {
        const VertexElement& pe = a.elements[0];
        if ((a.regs & 1) && pe.format == 2 && pe.buffer < a.buffer_count && a.buffers[pe.buffer].src) {
            const VertexBuffer& vb = a.buffers[pe.buffer];
            float x0 = 1e30f, x1 = -1e30f, y0 = 1e30f, y1 = -1e30f;
            uint32_t n = vb.stride ? std::min<uint32_t>(vb.bytes / vb.stride, 4) : 0;
            for (uint32_t i = 0; i < n; i++) {
                float xy[2];
                std::memcpy(xy, vb.src + size_t(i) * vb.stride + pe.offset, 8);
                x0 = std::min(x0, xy[0]);
                x1 = std::max(x1, xy[0]);
                y0 = std::min(y0, xy[1]);
                y1 = std::max(y1, xy[1]);
            }
            float w = float(rt->gw), h = float(rt->gh);
            auto short_by = [](float lo, float hi, float size) { return lo <= 0.5f && hi > size - 3.0f && hi < size - 0.5f; };
            if (n == 4 && short_by(x0, x1, w) && short_by(y0, y1, h)) {
                c.ext[0] = (w - x0) / (x1 - x0);
                c.ext[1] = x0;
                c.ext[2] = (h - y0) / (y1 - y0);
                c.ext[3] = y0;
            }
        }
    }
    if (a.pretransformed && rt && rt->gw >= 640) {
        float k = pk_.hud_shape;
        const VertexElement& pe = a.elements[0];
        if ((a.regs & 1) && pe.format == 2 && pe.buffer < a.buffer_count && a.buffers[pe.buffer].src) {
            const VertexBuffer& vb = a.buffers[pe.buffer];
            float x0 = 1e30f, x1 = -1e30f, y0 = 1e30f, y1 = -1e30f;
            uint32_t n = vb.stride ? std::min<uint32_t>(vb.bytes / vb.stride, 4096) : 0;
            for (uint32_t i = 0; i < n; i++) {
                float xy[2];
                std::memcpy(xy, vb.src + size_t(i) * vb.stride + pe.offset, 8);
                x0 = std::min(x0, xy[0]);
                x1 = std::max(x1, xy[0]);
                y0 = std::min(y0, xy[1]);
                y1 = std::max(y1, xy[1]);
            }
            float w = float(rt->gw), h = float(rt->gh);
            bool rt_tex = from_target[0];
            if (n && s.textures[0] && !rt_tex) {   // this frame's 2D picture area, for the next frame's mode
                float cw = std::clamp(x1, 0.0f, w) - std::clamp(x0, 0.0f, w);
                float ch = std::clamp(y1, 0.0f, h) - std::clamp(y0, 0.0f, h);
                if (cw > 0 && ch > 0) hud_cover_ += cw * ch / (w * h);
                if (cw > 0 && ch > 0 && paused_now_) paused_cover_ += cw * ch / (w * h);
            }
            // NGB_HUD_LOG=<file>: while it exists, one frame's screen-space draws (debugging)
            static const char* hudfile = std::getenv("NGB_HUD_LOG");
            static uint32_t hudframe = 0;
            if (hudfile && n) {
                if (!hudframe && access(hudfile, F_OK) == 0) { hudframe = presents_ + 1; std::remove(hudfile); }
                if (hudframe == presents_)
                    std::fprintf(stderr, "[HUD] frame %u menu %d n %u x %.0f..%.0f y %.0f..%.0f tex %08X%s\n", presents_,
                                 hud_menu_, n, x0, x1, y0, y1, s.textures[0], rt_tex ? " (target)" : "");
                else if (hudframe && presents_ > hudframe)
                    hudframe = 0;   // again when the file is made again
            }
            // NGB_HUD_RATIO_LOG=1 (debugging): each distinct 2D draw not drawn texel for pixel, its width to its
            // height against its texels' (the game's letters are 0.85 by design)
            static int rlog = std::getenv("NGB_HUD_RATIO_LOG") != nullptr;
            if (rlog && n && s.textures[0] && (a.regs >> 9) & 1) {
                const VertexElement& te = a.elements[9];
                TextureDesc td;
                if (te.format == 16 && te.buffer < a.buffer_count && a.buffers[te.buffer].src && Describe(s.textures[0], td)) {
                    const VertexBuffer& tb = a.buffers[te.buffer];
                    float u0 = 1e30f, u1 = -1e30f, v0 = 1e30f, v1 = -1e30f;
                    for (uint32_t i = 0; i < n; i++) {
                        float uv[2];
                        std::memcpy(uv, tb.src + size_t(i) * tb.stride + te.offset, 8);
                        u0 = std::min(u0, uv[0]); u1 = std::max(u1, uv[0]);
                        v0 = std::min(v0, uv[1]); v1 = std::max(v1, uv[1]);
                    }
                    float tw = (u1 - u0) * (td.linear ? 1.0f : float(td.width));
                    float th = (v1 - v0) * (td.linear ? 1.0f : float(td.height));
                    if (tw > 8 && th > 8) {
                        float r = ((x1 - x0) / tw) / ((y1 - y0) / th);
                        if (r < 0.95f || r > 1.05f) {
                            static std::unordered_map<uint64_t, int> seen;
                            uint64_t key = uint64_t(s.textures[0]) << 20 ^ uint64_t(int(r * 100)) << 8 ^ uint64_t(int(y0));
                            if (!seen[key]++)
                                std::fprintf(stderr, "[RATIO] %.2f tex %08X x %.0f..%.0f y %.0f..%.0f texels %.0fx%.0f\n", r,
                                             s.textures[0], x0, x1, y0, y1, tw, th);
                        }
                    }
                }
            }
            bool paused = rt_tex && x1 - x0 >= w * 0.99f;   // the paused game behind a menu: its 3D as it is
            if (paused && rt->gw >= 640) paused_now_ = true;
            // what is opaque over the whole screen hides all drawn before it
            if ((pipe_k.wmask & 7) == 7 && y0 <= 1 && y1 >= h - 1 && ((a.regs >> 3) & 1)) {
                const VertexElement& de = a.elements[3];
                uint32_t d = 0;
                if (de.buffer < a.buffer_count && a.buffers[de.buffer].src)
                    std::memcpy(&d, a.buffers[de.buffer].src + de.offset, 4);
                bool opaque = d >> 24 == 0xFF || !pipe_k.blend;
                // a colour: the pause menu covers the paused game so for its own art (the bars kept); in play, an
                // item's or a tip's screen covers the game so, its art all about the middle after it (HudPlace)
                if (opaque && !s.textures[0] && x0 <= 1 && x1 >= w - 1) {
                    paused_now_ = false;
                    covered_now_ = true;
                }
                // a picture (in pieces side by side): a chapter's clear and its card, over the last picture of the
                // level just ended -- a 2D screen, though the level's 3D was drawn this frame (a tip's, an item's
                // screen over a black cover is see-through, the game's: those fill the screen as in play)
                if (opaque && s.textures[0] && !rt_tex) {
                    if (x0 <= 1) pic_x1_ = x1;
                    else if (s.textures[0] == pic_tex_ && x0 <= pic_x1_ + 1) pic_x1_ = std::max(pic_x1_, x1);
                    pic_tex_ = s.textures[0];
                    if (x0 <= pic_x1_ + 1 && pic_x1_ >= w - 4) {
                        threed_ = 0;
                        ngb_frame_3d_draws = 0;
                    }
                }
            }
            if (pk_.stretched) {
                // The game's 2D in its widescreen mode, the console's 16:9 look, kept 16:9: on a wider screen
                // (21:9) narrowed to the middle 16:9 (kx), on a narrower one (16:10) shortened to it (ky);
                // a 2D screen (menus, the title, chapter cards) whole, clipped there; in play the HUD's
                // elements to the screen's sides and corners (HudPlace)
                float km = std::min(1.0f, k * 4.0f / 3.0f), ky = std::min(1.0f, 0.75f / k);
                bool spans = !s.textures[0] && x0 <= 1 && x1 >= w - 1;   // a fade, a letterbox bar: the screen
                // in play, a screen effect -- the water's surface seen from under it, a flash, a vignette, a
                // distortion reading the frame (a render target) -- fills the screen as drawn: anything
                // covering most of the screen, or reading a render target, is the 3D's, not the HUD's
                if (!ScreenFrame(threed_) && (rt_tex || (x1 - x0 >= w * 0.85f && y1 - y0 >= h * 0.75f))) spans = true;
                static int centred = -1;
                if (centred < 0) centred = std::getenv("NGB_HUD_CENTER") != nullptr;
                if (n && ky < 0.999f && !paused && !spans) {
                    float ax = w / 2, ay = h / 2;
                    if (!ScreenFrame(threed_) && !centred) {
                        HudPlace(x0, x1, y0, y1, w, h, ax, ay);
                        if (x1 - x0 < w * 0.89f && y1 - y0 < h * 0.9f && hud_boxes_.size() < 2048)
                            hud_boxes_.push_back({x0, x1, y0, y1, st_.drawn});
                    }
                    c.ext[0] = 1;
                    c.ext[1] = w / 2;
                    c.ext[2] = ky;
                    c.ext[3] = ay;
                    if (ax == w / 2 && ay == h / 2) {
                        hud_clipy0 = h / 2 * (1 - ky);
                        hud_clipy1 = h / 2 * (1 + ky);
                    }
                } else if (n && km < 0.999f && !paused && !spans) {
                    c.ext[0] = km;
                    c.ext[1] = w / 2;
                    // In play the HUD's elements to the screen's sides (HudPlace). NGB_HUD_CENTER=1: all about
                    // the middle.
                    if (!centred && !ScreenFrame(threed_)) {
                        float ax = w / 2, ay = h / 2;
                        HudPlace(x0, x1, y0, y1, w, h, ax, ay);
                        c.ext[1] = ax;
                        if (x1 - x0 < w * 0.89f && y1 - y0 < h * 0.9f && hud_boxes_.size() < 2048)
                            hud_boxes_.push_back({x0, x1, y0, y1, st_.drawn});
                    }
                    if (c.ext[1] == w / 2) {   // (a corner's piece is on the screen as narrowed: not clipped)
                        hud_clip0 = w / 2 * (1 - km);
                        hud_clip1 = w / 2 * (1 + km);
                    }
                }
            } else if (n && k < 0.999f && !paused &&
                       (ScreenFrame(threed_) || ((hud_menu_ || hud_cover_ >= 0.5f) && threed_ < 30))) {
                // A 2D screen in the game's 4:3 mode (Menus 4:3; or the first frame of one, before the game
                // switches): the console's 4:3 picture, all of it narrowed alike about the middle and clipped
                // there, black beyond (its pieces -- tiles, scrolling layers reaching off the screen -- only
                // line up drawn alike)
                c.ext[0] = k;
                c.ext[1] = w / 2;
                hud_clip0 = w / 2 * (1 - k);
                hud_clip1 = w / 2 * (1 + k);
            } else if (n && k < 0.999f && !rt_tex && x1 - x0 < w * 0.89f && x1 > 0 && x0 < w) {
                // the HUD along the top (the health bar, ninpo, the projectile icon under them) keeps to
                // its corner as on a 4:3 screen: narrowed toward the left edge, or the right edge for
                // what starts in the right part; everything lower (words, Game Over, subtitles, area
                // names, drawn in pieces across the screen) about the middle, together
                c.ext[0] = k;
                // and the tall gauges at a side (the breath bar under water) keep to that side
                bool gauge = y1 - y0 > h * 0.15f && x1 - x0 < w * 0.1f;
                if (gauge && gauges_now_ < 4) {
                    gauge_x_[gauges_now_][0] = x0;
                    gauge_x_[gauges_now_][1] = x1;
                    gauges_now_++;
                }
                // the gauge's caps and frame pieces (short, in its column, this frame or the one before)
                for (int g = 0; g < 4 && !gauge; g++)
                    if (x1 - x0 < w * 0.1f && y0 > h * 0.25f && x1 > gauge_prev_[g][0] - 4 && x0 < gauge_prev_[g][1] + 4)
                        gauge = true;
                c.ext[1] = y1 < h * 0.3f ? (x0 < w * 0.7f ? 0.0f : w)
                         : gauge && x1 < w * 0.3f ? 0.0f : gauge && x0 > w * 0.7f ? w : w / 2;
            }
        }
    }
    if (rt && rt->gw >= 640) ngb_frame_draws++;
    if (!a.pretransformed && rt && rt->gw < 640) offscreen3d_++;
    if (!a.pretransformed && rt && rt->gw >= 640) {
        static const bool viewlog = std::getenv("NGB_D3D_VIEWLOG") != nullptr;   // debugging: the camera's direction
        if (viewlog) {   // the view matrix's third column: where the camera looks, in the world
            const float* v = s.transforms[2].data();
            view_yaw_ = std::atan2(v[2], v[10]) * 57.2958f;
            view_pitch_ = std::asin(std::clamp(v[6], -1.0f, 1.0f)) * 57.2958f;
            view_seen_ = true;
        }
        threed_++;
        ngb_frame_3d_draws++;
        last3d_now_ = st_.drawn;
    }
    if (int(presents_) == g_step_frame && !dump_dir_.empty() && !prog && !a.pretransformed) {
        // the stepped frame (NGB_D3D_STEP): each fixed-function draw's matrices and raw positions, to compare two
        // meshes' depths the console's way and this renderer's (dump_dir/vertices.txt)
        static FILE* vf = nullptr;
        static int vf_frame = -1;
        if (vf_frame != g_step_frame) {
            if (vf) std::fclose(vf);
            vf = std::fopen((dump_dir_ + "/vertices_" + std::to_string(presents_) + ".txt").c_str(), "w");
            vf_frame = g_step_frame;
        }
        const VertexElement& pe = a.elements[0];
        if (vf && pe.buffer < a.buffer_count && a.buffers[pe.buffer].src && a.buffers[pe.buffer].stride) {
            const auto& pb = a.buffers[pe.buffer];
            uint32_t nv = pb.bytes / pb.stride;
            std::fprintf(vf, "draw %u tex0 %08X n %u verts %u fmt %u zen %u zw %u zf %X stack", st_.drawn + 1,
                         s.textures[0], count, nv, pe.format, uint32_t(s.rs[kRsZEnable]), SimpleState(s, kMZWriteEnable),
                         SimpleState(s, kMZFunc));
            for (int k2 = 0; k2 < g_draw_stack_n; k2++) std::fprintf(vf, " %08X", g_draw_stack[k2]);
            std::fprintf(vf, "\n");
            if (pk_.draw_node >= 0x10000 && pk_.draw_node < 0x4000000) {
                std::fprintf(vf, "node %08X", pk_.draw_node);
                for (uint32_t o = 0; o < 0x60; o += 4) std::fprintf(vf, " %08X", kernel::R32(pk_.draw_node + o));
                std::fprintf(vf, "\n");
            }
            for (int m : {256, 2, 3}) {
                std::fprintf(vf, "m%d", m);
                for (float x : s.transforms[size_t(m)]) std::fprintf(vf, " %.9g", x);
                std::fprintf(vf, "\n");
            }
            std::fprintf(vf, "vp %u %u %u %u %.9g %.9g zscale %.9g\n", s.viewport.x, s.viewport.y, s.viewport.w,
                         s.viewport.h, s.viewport.min_z, s.viewport.max_z, s.zscale);
            for (uint32_t v = 0; v < nv; v++) {
                float p[3];
                std::memcpy(p, pb.src + size_t(v) * pb.stride + pe.offset, 12);
                std::fprintf(vf, "v %.9g %.9g %.9g\n", p[0], p[1], p[2]);
            }
            if (a.index_src && a.index_count) {
                std::fprintf(vf, "i %u %d", a.index_count, a.base_vertex);
                for (uint32_t k2 = 0; k2 < a.index_count; k2++)
                    std::fprintf(vf, " %u", a.index16 ? uint32_t(reinterpret_cast<const uint16_t*>(a.index_src)[k2])
                                                       : reinterpret_cast<const uint32_t*>(a.index_src)[k2]);
                std::fprintf(vf, "\n");
            }
            std::fflush(vf);
        }
    }
    if (!prog && !a.pretransformed) {
        float wv[16], wvp[16], full[16];
        MatMul(s.transforms[256].data(), s.transforms[2].data(), wv);
        MatMul(wv, s.transforms[3].data(), wvp);
        const auto& v = s.viewport;
        float sx = float(v.w) / gw, sy = float(v.h) / gh;
        // half a console pixel right and down: its pixel centres are whole coordinates (gpu/shaders.cpp)
        float ox = (2.0f * v.x + v.w + 1.0f) / gw - 1, oy = 1 - (2.0f * v.y + v.h + 1.0f) / gh;
        float vpm[16] = {sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, v.max_z - v.min_z, 0, ox, oy, v.min_z, 1};
        MatMul(wvp, vpm, full);
        Transpose(full, c.wvp);
        Transpose(wv, c.wv);
        float n[16] = {};
        InverseTranspose3x3(wv, n);
        Transpose(n, c.wvn);
        if (fk.blend) {
            for (int m = 0; m < 4; m++) {
                float wvm[16], nm[16] = {};
                MatMul(s.transforms[256 + size_t(m)].data(), s.transforms[2].data(), wvm);
                Transpose(wvm, c.wvb[m]);
                InverseTranspose3x3(wvm, nm);
                Transpose(nm, c.wvbn[m]);
            }
            float pvm[16];
            MatMul(s.transforms[3].data(), vpm, pvm);
            Transpose(pvm, c.pv);
        }
        for (int t = 0; t < 4; t++) Transpose(s.transforms[16 + size_t(t)].data(), c.tm[t]);
        // lights in view space, the material and the ambient colour
        std::memcpy(c.mat, s.material.data(), sizeof(float) * 16);
        c.mat[4][0] = s.material[16];
        Color(s.rs[kRsAmbient], c.amb);
        for (int i = 0; i < 8; i++)
            if ((fk.types >> (i * 2)) & 3) LightToView(s.lights[size_t(i)], s.transforms[2].data(), c.lt[i]);
    }
    VkDeviceSize vc;
    if (points && !prog) {   // the point sizes for PointHlsl, in registers fixed function leaves unused
        auto v = s.vsc;
        float vh = s.viewport.h > 0 ? float(s.viewport.h) : float(gh);
        float mx = Bits(s.rs[kRsPointSizeMax]);
        if (!(mx > 0)) mx = 64.0f;
        v[190] = {Bits(s.rs[kRsPointSize]), Bits(s.rs[kRsPointSizeMin]), std::min(mx, point_max_ / c.surf[3]),
                  s.rs[kRsPointScaleEnable] ? 1.0f : 0.0f};
        v[191] = {Bits(s.rs[kRsPointScaleA]), Bits(s.rs[kRsPointScaleB]), Bits(s.rs[kRsPointScaleC]), vh};
        vc = Upload(v.data(), sizeof(v), ringAlign_);
    } else {
        vc = Upload(s.vsc.data(), sizeof(s.vsc), ringAlign_);
    }
    QuadSnap(s, a, count, quad, rt ? rt->samples : ds ? ds->samples : 1, c);
    bool id_opaque = !a.pretransformed && k.zen && k.zwrite && !k.blend && a.topology == Topology::Triangles &&
                     rt && rt->gw >= 640;
    bool id_dumping = !dump_dir_.empty() && presents_ + 1 >= dump_from_ && presents_ + 1 < dump_from_ + dump_count_;
    if (IdViewMode() == 2 && id_dumping) {   // only while a burst dumps: the game looks as usual otherwise
        // NGB_D3D_IDVIEW=2: only the opaque 3D draws, each writing its number (the frame's opaque draws counted
        // from 1); the rest skipped, so nothing blends over the numbers. With a dump running, each number's draw
        // goes to dump_dir/ids_<frame>.txt.
        if (!id_opaque) return;
        uint32_t n = ++id_count_;
        c.qsnap[2] = -1.0f;
        c.qsnap[3] = float(n);
        {
            if (id_file_frame_ != presents_ + 1) {
                if (id_file_) std::fclose(id_file_);
                id_file_ = std::fopen((dump_dir_ + "/ids_" + std::to_string(presents_ + 1) + ".txt").c_str(), "w");
                id_file_frame_ = presents_ + 1;
            }
            if (id_file_) {
                std::fprintf(id_file_, "%u piece %08X chunk %d tex %08X n %u", n, pk_.draw_node, pk_.chunk_slot,
                             s.textures[0], count);
                if (pk_.draw_node >= 0x10000 && pk_.draw_node < 0x4000000)
                    for (uint32_t o = 0; o < 0x20; o += 4) std::fprintf(id_file_, " %08X", kernel::R32(pk_.draw_node + o));
                std::fprintf(id_file_, "\n");
            }
        }
    } else if (IdView() && id_opaque) {
        // NGB_D3D_IDVIEW: one of 24 hues by the map piece and texture (or the draw's vertex buffer), the same every
        // frame; the legend, each colour's draws, goes to the log every 5 s
        uint32_t piece = pk_.draw_node, tex = s.textures[0];
        uint32_t key = piece ? piece * 2654435761u ^ tex * 40503u : uint32_t(s.streams[0].vb) * 2654435761u ^ tex;
        uint32_t idx = (key >> 7) % 24;
        c.qsnap[2] = float(idx + 1);
        static std::map<uint32_t, std::string> legend;
        static uint32_t last = 0;
        char line[160];
        std::snprintf(line, sizeof line, "piece %08X chunk %d tex %08X n %u", piece, pk_.chunk_slot, tex, count);
        std::string& e = legend[idx];
        if (e.find(line) == std::string::npos && e.size() < 600) e += std::string(e.empty() ? "" : "; ") + line;
        if (presents_ - last >= 300) {
            last = presents_;
            static const char* names[24] = {"red", "red-orange", "orange", "amber", "yellow", "lime", "yellow-green",
                                            "green", "green2", "spring", "mint", "teal", "cyan", "sky", "azure",
                                            "blue", "deep blue", "indigo", "violet", "purple", "magenta", "pink",
                                            "rose", "crimson"};
            std::fprintf(stderr, "[IDVIEW] frame %u legend:\n", presents_);
            for (auto& [i, t] : legend) std::fprintf(stderr, "[IDVIEW]   %2u %-12s %s\n", i + 1, names[i], t.c_str());
            std::fflush(stderr);
            legend.clear();
        }
    }
    VkDeviceSize dc = Upload(&c, sizeof(c), ringAlign_);

    // the vertices and indices
    VkDeviceSize voff[16];
    for (uint32_t b = 0; b < a.buffer_count; b++) voff[b] = Upload(a.buffers[b].src, a.buffers[b].bytes, 16);
    VkDeviceSize ioff = 0;
    uint32_t ibytes = a.index_count * (a.index16 ? 2 : 4);
    if (a.index_count) ioff = Upload(a.index_src, ibytes, 4);

    // record: the uploads above may have submitted, so everything is set after them
    BeginRendering(rt, ds);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorBufferInfo bvc{ring_, vc, sizeof(s.vsc)}, bdc{ring_, dc, sizeof(c)};
    VkDescriptorImageInfo img[4], smp[4];
    for (int i = 0; i < 4; i++) {
        VkImageView v = views[i];
        if (!v) v = info[i].kind == kTexCube ? nullcube_.view : null2d_.view;
        img[i] = {VK_NULL_HANDLE, v,
                  info[i].shadow ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        if (!views[i]) img[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkSampler sampler=glow_pass && i==0 ? glow_soft_sampler_ : Sampler(s,i,info[i]);
        smp[i] = {sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    }
    VkWriteDescriptorSet w[10] = {};
    for (int i = 0; i < 10; i++) {
        w[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w[i].dstBinding = uint32_t(i);
        w[i].descriptorCount = 1;
        if (i < 2) {
            w[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            w[i].pBufferInfo = i == 0 ? &bvc : &bdc;
        } else if (i < 6) {
            w[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            w[i].pImageInfo = &img[i - 2];
        } else {
            w[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            w[i].pImageInfo = &smp[i - 6];
        }
    }
    d_.vkCmdPushDescriptorSetKHR(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 10, w);

    // the viewport covers the target (the title's viewport is in the shader), upside down for
    // Direct3D's clip space; the scissor clips to the title's viewport and scissor
    const Image& any = rt ? rt->img : ds->img;
    VkViewport vpt{0, float(any.h), float(any.w), -float(any.h), 0, 1};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vpt);
    float sx = float(any.w) / gw, sy = float(any.h) / gh;
    int x0 = int(s.viewport.x * sx), y0 = int(s.viewport.y * sy);
    int x1 = int((s.viewport.x + s.viewport.w) * sx), y1 = int((s.viewport.y + s.viewport.h) * sy);
    if (s.scissor_count) {
        x0 = std::max(x0, int(s.scissors[0] * sx));
        y0 = std::max(y0, int(s.scissors[1] * sy));
        x1 = std::min(x1, int(s.scissors[2] * sx));
        y1 = std::min(y1, int(s.scissors[3] * sy));
    }
    if (hud_clip1 > 0) {
        x0 = std::max(x0, int(std::floor(hud_clip0 * sx)));
        x1 = std::min(x1, int(std::ceil(hud_clip1 * sx)));
    }
    if (hud_clipy1 > 0) {
        y0 = std::max(y0, int(std::floor(hud_clipy0 * sy)));
        y1 = std::min(y1, int(std::ceil(hud_clipy1 * sy)));
    }
    x0 = std::clamp(x0, 0, int(any.w));
    y0 = std::clamp(y0, 0, int(any.h));
    x1 = std::clamp(x1, x0, int(any.w));
    y1 = std::clamp(y1, y0, int(any.h));
    VkRect2D sc{{x0, y0}, {uint32_t(x1 - x0), uint32_t(y1 - y0)}};
    static int logn = std::getenv("NGB_D3D_LOGDRAWS") ? std::atoi(std::getenv("NGB_D3D_LOGDRAWS")) : 0;
    if (logn > 0) {
        logn--;
        std::fprintf(stderr, "[D3D] draw prim %u count %u regs %04X pre %d prog %d buffers %u index %u target %ux%u "
                     "viewport %u %u %u %u scissor %d %d %d %d c58 %g %g %g c59 %g %g %g\n", prim, count, a.regs,
                     a.pretransformed, prog, a.buffer_count, a.index_count, any.w, any.h, s.viewport.x, s.viewport.y,
                     s.viewport.w, s.viewport.h, x0, y0, x1, y1, s.vsc[58][0], s.vsc[58][1], s.vsc[58][2], s.vsc[59][0],
                     s.vsc[59][1], s.vsc[59][2]);
        for (int r = 0; r < 16; r++)
            if ((a.regs >> r) & 1) {
                const VertexElement& e = a.elements[size_t(r)];
                const VertexBuffer& vb = a.buffers[e.buffer];
                std::fprintf(stderr, "      v%d fmt %u buf %u off %u stride %u:", r, e.format, e.buffer, e.offset, vb.stride);
                if (vb.src && (e.format == 2 || e.format == 6 || e.format == 16))
                    for (int k2 = 0; k2 < (e.format == 2 ? 4 : e.format == 6 ? 3 : 2); k2++) {
                        float f;
                        std::memcpy(&f, vb.src + e.offset + 4 * k2, 4);
                        std::fprintf(stderr, " %g", f);
                    }
                std::fprintf(stderr, "\n");
            }
    }
    d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    float bf[4];
    Color(SimpleState(s, kMBlendColor), bf);
    d_.vkCmdSetBlendConstants(cmd_, bf);
    d_.vkCmdSetStencilReference(cmd_, VK_STENCIL_FACE_FRONT_AND_BACK, SimpleState(s, kMStencilRef) & 0xFF);
    if (k.has_ds & kDsDynamicBias)
        d_.vkCmdSetDepthBias(cmd_, float(k.zbias + pull_steps), 0.0f, k.zslope + pull_slope);
    if (Lines(k)) {  // D3DRS_LINEWIDTH, in the game's pixels: at the target's scale
        float line_width = wide_lines_ ? std::clamp(Bits(s.rs[kRsLineWidth]) * sx, 1.0f, line_max_) : 1.0f;
        if (Bits(s.rs[kRsLineWidth]) > 1.0f)   // a wide line: the impact sparks
            line_width = std::max(1.0f, line_width * SparkThickness());
        d_.vkCmdSetLineWidth(cmd_, line_width);
    }

    VkBuffer bufs[16];
    for (uint32_t b = 0; b < a.buffer_count; b++) bufs[b] = ring_;
    if (a.buffer_count) d_.vkCmdBindVertexBuffers(cmd_, 0, a.buffer_count, bufs, voff);
    uint32_t visq = ~0u;   // in a visibility test: counted, in console pixels (render scale and samples out)
    if (vis_on_ && vis_pool_[slot_] && vis_q_[slot_].size() < kVisQueries) {
        visq = uint32_t(vis_q_[slot_].size());
        vis_q_[slot_].push_back({vis_test_, sx * sy * float(rt ? rt->samples : ds->samples)});
        d_.vkCmdBeginQuery(cmd_, vis_pool_[slot_], visq, shared_ ? VK_QUERY_CONTROL_PRECISE_BIT : 0);
    }
    if (a.index_count) {
        d_.vkCmdBindIndexBuffer(cmd_, ring_, ioff, a.index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        d_.vkCmdDrawIndexed(cmd_, a.index_count, 1, 0, a.base_vertex, 0);
    } else {
        d_.vkCmdDraw(cmd_, a.draw_count, 1, 0, 0);
    }
    if (visq != ~0u) d_.vkCmdEndQuery(cmd_, vis_pool_[slot_], visq);
    if (rt && ContinuousGlowEnabled()) ApplyContinuousGlow(s,a,count,*rt,c);
    // NGB_D3D_STEP=frame,every: the target after every so many draws of that frame
    static int step_every = -1;
    if (step_every == -1) {
        step_every = 25;
        if (const char* e = std::getenv("NGB_D3D_STEP")) std::sscanf(e, "%d,%d", &g_step_frame, &step_every);
    }
    static bool trace_pre_only = std::getenv("NGB_TRACE_PRE") != nullptr;   // only screen-space draws
    if (trace_ && rt && rt->gw == 512 && rt->gh == 512 && from_target[0]) trace_pic_until_ = presents_ + 4;
    static uint32_t trace_last_rt = 0;
    bool rt_change = s.rt.data != trace_last_rt;
    trace_last_rt = s.rt.data;
    if (trace_ && (!trace_pre_only || a.pretransformed || rt_change)) {
        TextureDesc t0d, t1d;
        bool h0 = s.textures[0] && Describe(s.textures[0], t0d), h1 = s.textures[1] && Describe(s.textures[1], t1d);
        std::fprintf(trace_, "%u %u prim %u n %u pre %d prog %d ps %08X blend %u %X %X %X wm %X z %u%u%X st %u "
                     "rt %08X %ux%u tex0 %08X @%08X %X %ux%u%s tex1 %08X %X tss0 %u %u %u %u|%u %u %u tss1 %u %u %u|%u "
                     "tf %08X modes %08X vs %08X ext %.2f fog %u mode %u col %08X %g..%g d0 %08X signed %u%u%u%u uv %s "
                     "shaders %016llx %016llx light %X spec %g %g %g pow %g csign %X %X %X %X lw %08X "
                     "pt %u %u %g %g %g %u %g %g %g regs %X\n",
                     presents_, st_.drawn, prim, count, a.pretransformed, prog, s.ps ? s.ps->handle : 0, k.blend, k.src,
                     k.dst, k.op, k.wmask, k.zen, k.zwrite, k.zfunc, k.sten, s.rt.data, rt ? rt->gw : 0, rt ? rt->gh : 0,
                     s.textures[0], h0 ? t0d.data : 0, h0 ? t0d.format : 0, h0 ? t0d.width : 0, h0 ? t0d.height : 0,
                     from_target[0] ? " (target)" : "", s.textures[1], h1 ? t1d.format : 0, s.tss[0][kTssColorOp],
                     s.tss[0][kTssColorArg1], s.tss[0][kTssColorArg2], s.tss[0][kTssColorArg0], s.tss[0][kTssAlphaOp],
                     s.tss[0][kTssAlphaArg1], s.tss[0][kTssAlphaArg2], s.tss[1][kTssColorOp], s.tss[1][kTssColorArg1],
                     s.tss[1][kTssColorArg2], s.tss[1][kTssAlphaOp], s.rs[kRsTextureFactor], s.rs[kRsPsTextureModes],
                     s.vs_handle, c.ext[0], s.rs[kRsFogEnable], s.rs[kRsFogTableMode], s.rs[kRsFogColor],
                     Bits(s.rs[kRsFogStart]), Bits(s.rs[kRsFogEnd]), [&]() -> uint32_t {
                         if (!((a.regs >> 3) & 1)) return 0xFFFFFFFFu;   // no diffuse: none given
                         const VertexElement& e = a.elements[3];
                         uint32_t v = 0;
                         if (e.buffer < a.buffer_count && a.buffers[e.buffer].src)
                             std::memcpy(&v, a.buffers[e.buffer].src + e.offset, 4);
                         return v;
                     }(), (info[0].flags & kTexSigned) != 0, (info[1].flags & kTexSigned) != 0,
                     (info[2].flags & kTexSigned) != 0, (info[3].flags & kTexSigned) != 0,
                     [&]() -> std::string {   // each vertex: x y (u v)
                         std::string o;
                         const VertexElement& pe = a.elements[0];
                         const VertexElement& te = a.elements[9];
                         uint32_t n = 0;
                         if (pe.buffer < a.buffer_count && a.buffers[pe.buffer].stride)
                             n = std::min<uint32_t>(a.buffers[pe.buffer].bytes / a.buffers[pe.buffer].stride, 4);
                         for (uint32_t i = 0; i < n; i++) {
                             float xy[2] = {}, uv[2] = {};
                             std::memcpy(xy, a.buffers[pe.buffer].src + size_t(i) * a.buffers[pe.buffer].stride + pe.offset, 8);
                             if (((a.regs >> 9) & 1) && te.buffer < a.buffer_count && a.buffers[te.buffer].src)
                                 std::memcpy(uv, a.buffers[te.buffer].src + size_t(i) * a.buffers[te.buffer].stride + te.offset, 8);
                             char b[64];
                             std::snprintf(b, sizeof b, "%s%.0f,%.0f(%.3g,%.3g)", i ? " " : "", xy[0], xy[1], uv[0], uv[1]);
                             o += b;
                         }
                         return o;
                     }().c_str(), (unsigned long long)k.vs, (unsigned long long)k.ps,   // NGB_D3D_SHADERDUMP's names
                     prog ? 0u : MakeFixedKey(s).light, s.material[8], s.material[9], s.material[10], s.material[16],
                     s.tss[0][kTssColorSign] >> 28, s.tss[1][kTssColorSign] >> 28, s.tss[2][kTssColorSign] >> 28,
                     s.tss[3][kTssColorSign] >> 28,   // D3DTSS_COLORSIGN: A R G B signed, bits 0-3
                     s.rs[kRsLineWidth],
                     // points: sprite on, scaling on, size, min, max (D3D's, before the render scale), A B C, the
                     // vertex's attributes (bit 9: texture coordinates 0; #51)
                     s.rs[kRsPointSpriteEnable], s.rs[kRsPointScaleEnable], Bits(s.rs[kRsPointSize]),
                     Bits(s.rs[kRsPointSizeMin]), Bits(s.rs[kRsPointSizeMax]), prim == 1 ? 1u : 0u,
                     Bits(s.rs[kRsPointScaleA]), Bits(s.rs[kRsPointScaleB]), Bits(s.rs[kRsPointScaleC]), a.regs);
    }
    if (int(presents_) == g_step_frame)
        std::fprintf(stderr, "[STEP] %u prim %u count %u pre %d prog %d ps %08X blend %u %X %X op %X wmask %X zen %u "
                     "zw %u zf %X sten %u cull %X front %X tex %08X %08X rt %08X ds %08X idx %u\n", st_.drawn, prim, count,
                     a.pretransformed, prog, s.ps ? s.ps->handle : 0, k.blend, k.src, k.dst, k.op, k.wmask, k.zen,
                     k.zwrite, k.zfunc, k.sten, s.rs[kRsCullMode], s.rs[kRsFrontFace], s.textures[0], s.textures[1],
                     s.rt.data, ds ? s.zs.data : 0, a.index_count);
    if (int(presents_) == g_step_frame) {
        TextureDesc d0;
        bool h0 = s.textures[0] && Describe(s.textures[0], d0);
        if (h0 && !dump_dir_.empty() && st_.drawn <= 40 && !from_target[0]) {
            char name[64];
            std::snprintf(name, sizeof(name), "step_%05u_%04u_tex0", presents_, st_.drawn);
            const uint32_t* pal = d0.format == 0x0B && s.palettes[0]
                ? static_cast<const uint32_t*>(HostPtr(ResourceDataOf(s.palettes[0]))) : nullptr;
            DumpTexture(d0, Format(d0.format), static_cast<const uint8_t*>(HostPtr(d0.data)), pal, dump_dir_.c_str(), name);
        }
        std::fprintf(stderr, "       rt format %08X zs format %08X tfactor %08X\n", s.rt.format, s.zs.format, s.rs[kRsTextureFactor]);
        std::fprintf(stderr, "       regs %04X light %u tss0 cop %u %u %u aop %u %u %u tci %u | tex0 fmt %X %ux%u lin %d view %d "
                     "tss1 cop %u | ps prog %d texmodes %X\n", a.regs, s.rs[kRsLighting], s.tss[0][kTssColorOp],
                     s.tss[0][kTssColorArg1], s.tss[0][kTssColorArg2], s.tss[0][kTssAlphaOp], s.tss[0][kTssAlphaArg1],
                     s.tss[0][kTssAlphaArg2], s.tss[0][kTssTexCoordIndex], h0 ? d0.format : 0, h0 ? d0.width : 0,
                     h0 ? d0.height : 0, h0 ? d0.linear : 0, views[0] != VK_NULL_HANDLE ? (from_target[0] ? 2 : 1) : 0, s.tss[1][kTssColorOp],
                     s.ps != nullptr, s.rs[kRsPsTextureModes]);
        if (info[0].shadow) {
            std::fprintf(stderr, "       tss0 raw:");
            for (int j = 0; j < 32; j++) std::fprintf(stderr, " %X", s.tss[0][j]);
            std::fprintf(stderr, "\n       tfactor %08X psmodes %08X\n", s.rs[kRsTextureFactor], s.rs[kRsPsTextureModes]);
            std::fprintf(stderr, "       shadow: func %X (vk %u) tex0 data %08X shadow %d viewport %u %u %u %u zscale %g\n",
                         s.rs[kRsShadowFunc], unsigned(Cmp(s.rs[kRsShadowFunc])), h0 ? d0.data : 0, info[0].shadow,
                         s.viewport.x, s.viewport.y, s.viewport.w, s.viewport.h, s.zscale);
            static uint32_t dumped_frame;
            if (!dump_dir_.empty() && dumped_frame != presents_) {
                dumped_frame = presents_;
                auto tit = targets_.find(uint64_t(d0.data & 0x7FFFFFFFu) << 1 | 1);
                if (tit != targets_.end()) {
                    char name[64];
                    std::snprintf(name, sizeof(name), "step_%05u_%04u_shadowmap", presents_, st_.drawn);
                    Dump(tit->second, name);
                    // rendering is over now: the draw's state is re-bound by the next draw
                }
            }
        }
        if (!prog) {
            std::fprintf(stderr, "       fixed light %X types %X enable %X ambient %08X mat d %g %g %g a %g %g %g e %g %g %g\n",
                         fk.light, fk.types, s.light_enable, s.rs[kRsAmbient], s.material[0], s.material[1], s.material[2],
                         s.material[4], s.material[5], s.material[6], s.material[12], s.material[13], s.material[14]);
            for (int i = 0; i < 8; i++)
                if ((fk.types >> (i * 2)) & 3) {
                    float f[26];
                    std::memcpy(f, s.lights[size_t(i)].data(), sizeof f);
                    std::fprintf(stderr, "       light %d type %g dif %g %g %g amb %g %g %g pos %g %g %g dir %g %g %g range %g att %g %g %g\n",
                                 i, f[0], f[1], f[2], f[3], f[9], f[10], f[11], f[13], f[14], f[15], f[16], f[17], f[18],
                                 f[19], f[21], f[22], f[23]);
                }
        }
        std::fprintf(stderr, "       vs %08X decl:", s.vs_handle);
        for (int r = 0; r < 16; r++)
            if ((a.regs >> r) & 1) {
                const VertexElement& e = a.elements[size_t(r)];
                uint32_t first = 0;
                if (a.buffers[e.buffer].src) std::memcpy(&first, a.buffers[e.buffer].src + e.offset, 4);
                std::fprintf(stderr, " v%d fmt %u off %u stride %u decode %X [%08X];", r, e.format, e.offset,
                             a.buffers[e.buffer].stride, unsigned((a.decode >> (4 * r)) & 15), first);
            }
        std::fprintf(stderr, "\n");
    }
    if (int(presents_) == g_step_frame && step_every > 0 && !dump_dir_.empty() && rt && (st_.drawn % step_every) == 0) {
        char name[64];
        std::snprintf(name, sizeof(name), "step_%05u_%04u_%08X", presents_, st_.drawn, s.rt.data);
        Dump(*rt, name);
    }
}

void VulkanBackend::Copy(const Surface& src, const Surface& dst, uint32_t count, const uint8_t* src_rects,
                         const uint8_t* dst_points) {
    if (ContinuousGlowEnabled()) InvalidateContinuousGlow(dst.data);
    Target* a = EnsureTarget(src, false);
    Target* b = EnsureTarget(dst, false);
    if (!a || !b || a == b) return;
    if (b->samples > 1) b->ms_stale = true;          // a copy into it: the plain image's picture, for MSAA too
    EndRendering();
    Barrier(a->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    Barrier(b->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    std::vector<VkImageBlit> rs;
    auto add = [&](int x0, int y0, int x1, int y1, int dx, int dy) {
        VkImageBlit r{};
        r.srcSubresource = r.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        float sa = float(a->img.w) / a->gw, sb = float(b->img.w) / b->gw;
        r.srcOffsets[0] = {int(x0 * sa), int(y0 * sa), 0};
        r.srcOffsets[1] = {int(x1 * sa), int(y1 * sa), 1};
        r.dstOffsets[0] = {int(dx * sb), int(dy * sb), 0};
        r.dstOffsets[1] = {int((dx + x1 - x0) * sb), int((dy + y1 - y0) * sb), 1};
        rs.push_back(r);
    };
    if (count && src_rects) {
        for (uint32_t i = 0; i < count; i++) {
            int32_t r[4], p[2];
            std::memcpy(r, src_rects + i * 16, 16);
            if (dst_points) std::memcpy(p, dst_points + i * 8, 8);
            int x0 = r[0], y0 = r[1], x1 = r[2], y1 = r[3];
            int dx = dst_points ? p[0] : x0;
            int dy = dst_points ? p[1] : y0;
            add(x0, y0, x1, y1, dx, dy);
        }
    } else {
        add(0, 0, int(std::min(a->gw, b->gw)), int(std::min(a->gh, b->gh)), 0, 0);
    }
    d_.vkCmdBlitImage(cmd_, a->img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, b->img.image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(rs.size()), rs.data(), VK_FILTER_LINEAR);
    b->drawn = true;
}

void VulkanBackend::Dump(Target& t, const char* name) {
    EndRendering();
    VkBuffer buf;
    VmaAllocation mem;
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = VkDeviceSize(t.img.w) * t.img.h * 4;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_AUTO;
    aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo info;
    if (vmaCreateBuffer(vma_, &bci, &aci, &buf, &mem, &info) != VK_SUCCESS) return;
    Barrier(t.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy c{};
    c.imageSubresource = {t.img.depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    c.imageExtent = {t.img.w, t.img.h, 1};
    d_.vkCmdCopyImageToBuffer(cmd_, t.img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &c);
    Submit();
    char path[1024];
    if (name)
        std::snprintf(path, sizeof(path), "%s/%s.ppm", dump_dir_.c_str(), name);
    else
        std::snprintf(path, sizeof(path), "%s/frame_%05u.ppm", dump_dir_.c_str(), presents_);
    if (FILE* f = std::fopen(path, "wb")) {
        std::fprintf(f, "P6\n%u %u\n255\n", t.img.w, t.img.h);
        const uint8_t* p = static_cast<const uint8_t*>(info.pMappedData);
        std::vector<uint8_t> row(size_t(t.img.w) * 3);
        for (uint32_t y = 0; y < t.img.h; y++) {
            for (uint32_t x = 0; x < t.img.w; x++) {
                const uint8_t* q = p + (size_t(y) * t.img.w + x) * 4;   // B G R A, or a depth value
                if (t.img.depth) {
                    // debugging: the depth as grey (24-bit fixed point in the low bits, or a float)
                    uint32_t v;
                    std::memcpy(&v, q, 4);
                    float z;
                    if (t.img.format == VK_FORMAT_D32_SFLOAT_S8_UINT) std::memcpy(&z, q, 4);
                    else z = float(v & 0xFFFFFF) / 16777215.0f;
                    row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = uint8_t(std::min(255.0f, z * 255.0f));
                    continue;
                }
                row[x * 3] = q[2];
                row[x * 3 + 1] = q[1];
                row[x * 3 + 2] = q[0];
            }
            std::fwrite(row.data(), 1, row.size(), f);
        }
        std::fclose(f);
        static bool alpha = std::getenv("NGB_D3D_DUMP_ALPHA") != nullptr;   // debugging: the alpha too, as PGM
        if (alpha && !t.img.depth) {
            std::string ap = std::string(path) + ".alpha.pgm";
            if (FILE* g = std::fopen(ap.c_str(), "wb")) {
                std::fprintf(g, "P5\n%u %u\n255\n", t.img.w, t.img.h);
                const uint8_t* q = static_cast<const uint8_t*>(info.pMappedData);
                for (size_t i = 0; i < size_t(t.img.w) * t.img.h; i++) std::fputc(q[i * 4 + 3], g);
                std::fclose(g);
            }
        }
    }
    vmaDestroyBuffer(vma_, buf, mem);
}

// Where a HUD element keeps to, by its box (guest pixels): about the middle when it is about the middle
// (subtitles, a message box), else the nearer side (the health bar and karma left, a prompt right, the breath
// bar at its side); the top when it starts near the top (the health bar, with the karma window open below it
// too), the bottom when it ends near the bottom, else the middle (on a narrower screen).
static void hud_side(float x0, float x1, float y0, float y1, float w, float h, float& ax, float& ay) {
    float cx = (x0 + x1) / 2;
    ax = std::fabs(cx - w / 2) < w * 0.12f ? w / 2 : cx < w / 2 ? 0.0f : w;
    bool top = y0 < h * 0.25f, bottom = y1 > h * 0.75f;
    ay = top && !bottom ? 0.0f : bottom && !top ? h : h / 2;
}

// A tall gauge at a side (the breath bar under water), or a piece in its column (its caps, its fill however
// low): an element of its own, never one with a window drawn over it (karma's, which reaches down over the
// bar's top -- together they spanned the screen's height and kept to its middle, the whole HUD moving).
static bool hud_gauge_piece(float x0, float x1, float y0, float y1, float w, float h, float gx0, float gx1,
                            float gy0, float gy1) {
    if (y1 - y0 > h * 0.15f && x1 - x0 < w * 0.1f) return true;
    return gx1 > gx0 && x0 >= gx0 - 4 && x1 <= gx1 + 4 && y0 >= gy0 - 14 && y1 <= gy1 + 14;
}

// A HUD piece in play: the side of the element it was part of the frame before (the smallest whose box holds
// its middle); a piece new this frame, along the top by its own box (the health bar as it appears), else
// about the middle until the next frame groups it.
void VulkanBackend::HudPlace(float x0, float x1, float y0, float y1, float w, float h, float& ax, float& ay) {
    // a screen of panels drawn over the game (a chapter's clear, an item's picture) whole, about the middle, as
    // a 2D screen
    float cx = (x0 + x1) / 2, cy = (y0 + y1) / 2, best = 1e30f;
    if (covered_now_) {   // over an opaque cover (an item's screen): no game, no HUD, the screen's art whole
        ax = w / 2;
        ay = h / 2;
        return;
    }
    // what is inside a panel screen's panel (its labels and figures, wherever in it) about the middle with it;
    // the HUD around it (the health bar, karma, prompts) where it is
    if (hud_panel_ && cx >= hud_panel_box_.x0 - 8 && cx <= hud_panel_box_.x1 + 8 && cy >= hud_panel_box_.y0 - 8 &&
        cy <= hud_panel_box_.y1 + 8) {
        ax = w / 2;
        ay = h / 2;
        return;
    }
    // a gauge's pieces to the gauge's element, the rest to the others: a gauge piece is tall, or one of the
    // frame before's gauge pieces (its fill shrinks slowly), or new in a gauge's column where no other
    // element is (a window sliding across the column is not the gauge's)
    bool gauge = hud_gauge_piece(x0, x1, y0, y1, w, h, 0, 0, 0, 0);
    for (const HudBox& g : hud_gauge_boxes_)
        if (!gauge && std::fabs(g.x0 - x0) <= 3 && std::fabs(g.x1 - x1) <= 3 && std::fabs(g.y0 - y0) <= 3 &&
            std::fabs(g.y1 - y1) <= 3)
            gauge = true;
    if (!gauge) {
        bool column = false, other = false;
        for (const HudElem& e : hud_elems_) {
            if (e.gauge && hud_gauge_piece(x0, x1, y0, y1, w, h, e.x0, e.x1, e.y0, e.y1)) column = true;
            if (!e.gauge && cx >= e.x0 - 2 && cx <= e.x1 + 2 && cy >= e.y0 - 2 && cy <= e.y1 + 2) other = true;
        }
        gauge = column && !other;
    }
    const HudElem* hit = nullptr;
    for (const HudElem& e : hud_elems_) {
        if (e.gauge != gauge) continue;
        if (cx < e.x0 - 2 || cx > e.x1 + 2 || cy < e.y0 - 2 || cy > e.y1 + 2) continue;
        float area = (e.x1 - e.x0) * (e.y1 - e.y0);
        if (area < best) { best = area; hit = &e; }
    }
    if (!hit)   // an element come back (a prompt shown again): where it was within the last half minute
        for (const HudElem& e : hud_memory_) {
            if (e.gauge != gauge) continue;
            if (cx < e.x0 - 2 || cx > e.x1 + 2 || cy < e.y0 - 2 || cy > e.y1 + 2) continue;
            float area = (e.x1 - e.x0) * (e.y1 - e.y0);
            if (area < best) { best = area; hit = &e; }
        }
    if (hit) {
        ax = hit->ax;
        ay = hit->ay;
        return;
    }
    hud_side(x0, x1, y0, y1, w, h, ax, ay);
    if (y1 >= h * 0.3f) ax = w / 2;
}

// Each frame's end: this frame's HUD pieces into elements -- pieces within a few pixels of each other (the
// letters of a word, a gauge, its caps and the words under it), together -- each with its side.
void VulkanBackend::HudGroup(float w, float h) {
    // only what was drawn after the 3D: the HUD (the 3D's screen-space pieces are not the HUD's)
    if (last3d_now_)
        hud_boxes_.erase(std::remove_if(hud_boxes_.begin(), hud_boxes_.end(),
                                        [&](const HudBox& b) { return b.at < last3d_now_; }), hud_boxes_.end());
    last3d_now_ = 0;
    size_t n = hud_boxes_.size();
    float cover = 0;
    for (const HudBox& b : hud_boxes_)
        cover += (std::clamp(b.x1, 0.0f, w) - std::clamp(b.x0, 0.0f, w)) * (std::clamp(b.y1, 0.0f, h) - std::clamp(b.y0, 0.0f, h));
    hud_box_cover_ = cover / (w * h);
    std::vector<int> up(n);
    for (size_t i = 0; i < n; i++) up[i] = int(i);
    auto root = [&](int i) { while (up[size_t(i)] != i) i = up[size_t(i)] = up[size_t(up[size_t(i)])]; return i; };
    const float gx = 10, gy = 14;
    // the tall gauges, then the pieces in their columns drawn with them (hud_gauge_piece; a few draws
    // apart -- not a window's letters sliding across): grouped only among themselves
    std::vector<char> gauge(n, 0);
    for (size_t i = 0; i < n; i++) {
        const HudBox& b = hud_boxes_[i];
        gauge[i] = hud_gauge_piece(b.x0, b.x1, b.y0, b.y1, w, h, 0, 0, 0, 0);
    }
    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n && !gauge[i]; j++) {
            const HudBox &b = hud_boxes_[i], &g = hud_boxes_[j];
            if (j != i && g.y1 - g.y0 > h * 0.15f && g.x1 - g.x0 < w * 0.1f && b.at + 6 >= g.at && g.at + 6 >= b.at &&
                hud_gauge_piece(b.x0, b.x1, b.y0, b.y1, w, h, g.x0, g.x1, g.y0, g.y1))
                gauge[i] = 1;
        }
    hud_gauge_boxes_.clear();
    for (size_t i = 0; i < n; i++)
        if (gauge[i]) hud_gauge_boxes_.push_back(hud_boxes_[i]);
    for (size_t i = 0; i < n; i++)
        for (size_t j = i + 1; j < n; j++) {
            const HudBox &a = hud_boxes_[i], &b = hud_boxes_[j];
            // a line of text's words (letters of one height, side by side) together across a space of up to
            // 0.6 letter heights (an item's name, "Great Spirit Elixir", was split in two at a 10-pixel gap)
            float g = std::fabs(a.y0 - b.y0) <= 2 && std::fabs(a.y1 - b.y1) <= 2 ? std::max(gx, (a.y1 - a.y0) * 0.6f) : gx;
            if (gauge[i] == gauge[j] && a.x0 - g <= b.x1 && b.x0 - g <= a.x1 && a.y0 - gy <= b.y1 &&
                b.y0 - gy <= a.y1)
                up[size_t(root(int(i)))] = root(int(j));
        }
    std::vector<HudElem> elems;
    std::vector<int> slot(n, -1);
    for (size_t i = 0; i < n; i++) {
        int r = root(int(i));
        const HudBox& b = hud_boxes_[i];
        if (slot[size_t(r)] < 0) {
            slot[size_t(r)] = int(elems.size());
            elems.push_back({b.x0, b.x1, b.y0, b.y1, 0, 0, 0, gauge[i] != 0});
        } else {
            HudElem& e = elems[size_t(slot[size_t(r)])];
            e.x0 = std::min(e.x0, b.x0); e.x1 = std::max(e.x1, b.x1);
            e.y0 = std::min(e.y0, b.y0); e.y1 = std::max(e.y1, b.y1);
        }
    }
    bool panel = false;
    HudBox pbox{1e30f, -1e30f, 1e30f, -1e30f, 0};
    for (HudElem& e : elems) {
        hud_side(e.x0, e.x1, e.y0, e.y1, w, h, e.ax, e.ay);
        e.seen = presents_;
        float ew = std::min(e.x1, w) - std::max(e.x0, 0.0f), eh = std::min(e.y1, h) - std::max(e.y0, 0.0f);
        // (tall and across the screen's middle: an area's name, a wide picture in a band, is not one; its
        // middle the on-screen part's -- an item's screen scrolls its background in pieces far past the sides)
        if (ew > w * 0.35f && eh > h * 0.45f && e.y0 < h * 0.35f && e.y1 > h * 0.65f &&
            std::fabs((std::max(e.x0, 0.0f) + std::min(e.x1, w)) / 2 - w / 2) < w * 0.12f) {
            panel = true;
            pbox.x0 = std::min(pbox.x0, e.x0); pbox.x1 = std::max(pbox.x1, e.x1);
            pbox.y0 = std::min(pbox.y0, e.y0); pbox.y1 = std::max(pbox.y1, e.y1);
        }
    }
    if (panel) {   // the panel's reach while it is seen (it slides and grows in)
        if (!hud_panel_ && hud_panel_count_ == 0) hud_panel_seen_ = pbox;
        hud_panel_seen_.x0 = std::min(hud_panel_seen_.x0, pbox.x0); hud_panel_seen_.x1 = std::max(hud_panel_seen_.x1, pbox.x1);
        hud_panel_seen_.y0 = std::min(hud_panel_seen_.y0, pbox.y0); hud_panel_seen_.y1 = std::max(hud_panel_seen_.y1, pbox.y1);
        hud_panel_box_ = hud_panel_seen_;
    }
    if (panel != hud_panel_) {
        if (++hud_panel_count_ >= (panel ? 4 : 8)) {
            hud_panel_ = panel;
            hud_panel_count_ = 0;
        }
    } else {
        hud_panel_count_ = 0;
    }
    if (!hud_panel_) {   // remembered: each element, the newest where they overlap
        for (const HudElem& e : elems) {
            hud_memory_.erase(std::remove_if(hud_memory_.begin(), hud_memory_.end(), [&](const HudElem& m) {
                return m.gauge == e.gauge && m.x0 < e.x1 && e.x0 < m.x1 && m.y0 < e.y1 && e.y0 < m.y1;
            }), hud_memory_.end());
            hud_memory_.push_back(e);
        }
        hud_memory_.erase(std::remove_if(hud_memory_.begin(), hud_memory_.end(),
                                         [&](const HudElem& m) { return presents_ - m.seen > 1800; }),
                          hud_memory_.end());
        if (hud_memory_.size() > 64) hud_memory_.erase(hud_memory_.begin(), hud_memory_.end() - 64);
    }
    static int trace = std::getenv("NGB_HUD_GROUP_TRACE") != nullptr;
    if (trace) {   // debugging: each change in what the HUD keeps to
        std::string sig = std::to_string(n) + (hud_panel_ ? " PANEL" : "");
        for (const HudElem& e : elems) {
            char b[96];
            std::snprintf(b, sizeof b, " [%.0f..%.0f,%.0f..%.0f %c%c%s]", e.x0, e.x1, e.y0, e.y1,
                          e.ax == 0 ? 'L' : e.ax == w ? 'R' : 'M', e.ay == 0 ? 'T' : e.ay == h ? 'B' : 'C',
                          e.gauge ? " gauge" : "");
            sig += b;
        }
        static std::string last;
        if (sig != last) std::fprintf(stderr, "[HUDTRACE] %u cover %.2f %s\n", presents_, hud_box_cover_, sig.c_str());
        last = sig;
    }
    static int log = std::getenv("NGB_HUD_GROUP_LOG") != nullptr;
    if (log && presents_ % 120 == 0)
        std::fprintf(stderr, "[HUDGROUP] cover %.2f\n", hud_box_cover_);
    if (log && presents_ % 120 == 0)
        for (const HudElem& e : elems)
            std::fprintf(stderr, "[HUDGROUP] %.0f..%.0f x %.0f..%.0f -> %s %s\n", e.x0, e.x1, e.y0, e.y1,
                         e.ax == 0 ? "left" : e.ax == w ? "right" : "middle", e.ay == 0 ? "top" : e.ay == h ? "bottom" : "centre");
    hud_elems_ = std::move(elems);
    hud_boxes_.clear();
}

void VulkanBackend::TraceFrame(Target& t) {
    const uint32_t w = 360, h = 240;
    if (!trace_img_.image)
        CreateImage(trace_img_, kColorFormat, w, h, 1, 1, false, false,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    if (!trace_img_.image) return;
    EndRendering();
    Barrier(t.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    Barrier(trace_img_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit b{};
    b.srcSubresource = b.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    b.srcOffsets[1] = {int32_t(t.img.w), int32_t(t.img.h), 1};
    b.dstOffsets[1] = {int32_t(w), int32_t(h), 1};
    d_.vkCmdBlitImage(cmd_, t.img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, trace_img_.image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &b, VK_FILTER_LINEAR);
    std::string saved = dump_dir_;
    dump_dir_ = trace_dir_;
    Target tt;
    tt.img = trace_img_;
    Dump(tt, nullptr);
    trace_img_.layout = tt.img.layout;
    dump_dir_ = saved;
}

void VulkanBackend::Present(const Surface& front) {
    if (!pk_given_) {
        static const State none;
        CapturePeek(none, pk_);
        CaptureQuick(qk_);
    }
    presents_++;
    id_count_ = 0;
    if (id_file_) std::fflush(id_file_);
    static const bool section_log = std::getenv("NGB_D3D_SECTION_LOG") != nullptr;
    if (SectionPull() > 0 && section_log && presents_ % 120 == 0)
        std::fprintf(stderr, "[SECTION] %u draws pulled\n", section_pulled_);
    section_pulled_ = 0;
    {   // NGB_TRACE_FILE=<file>: its appearing starts a recording too (headless tests)
        static const char* tf = std::getenv("NGB_TRACE_FILE");
        if (tf && access(tf, F_OK) == 0) {
            std::remove(tf);
            ngb_trace_request = 1;
        }
    }
    if (ngb_trace_request && !trace_) {   // F7: start recording
        ngb_trace_request = 0;
        const char* home = ngb_home();
        for (int n = 1; n < 1000 && !trace_; n++) {
            trace_dir_ = std::string(home ? home : ".") + "/ngb-trace-" + std::to_string(n);
            if (ngb_mkdir(trace_dir_.c_str()) == 0) trace_ = std::fopen((trace_dir_ + "/draws.txt").c_str(), "w");
        }
        trace_left_ = std::getenv("NGB_TRACE_FRAMES") ? std::atoi(std::getenv("NGB_TRACE_FRAMES")) : 300;
        std::fprintf(stderr, "[TRACE] recording %d frames to %s\n", trace_left_, trace_dir_.c_str());
    }
    static bool hitchlog = std::getenv("NGB_D3D_HITCHLOG") != nullptr;   // debugging: frames held up by compiling
    if (hitchlog && skipped_)
        std::fprintf(stderr, "[HITCH] frame %u: %u draws left out while their shaders are made\n", presents_,
                     skipped_.exchange(0));
    if (hitchlog && compile_us_ > 2000)
        std::fprintf(stderr, "[HITCH] frame %u: %.1f ms making %u shaders, %u pipelines\n", presents_, compile_us_ / 1000.0,
                     compiles_, pipes_made_);
    compile_us_ = 0;
    compiles_ = pipes_made_ = 0;
    // Every five minutes at most (and at exit): even off the frame, copying out a driver cache of tens of
    // MB (Nvidia) every ten seconds left a second of doubled frames each time (#17).
    // (The first after a minute: the save at exit does not run when the process ends without taking the
    // renderer down, and a session shorter than five minutes would never keep its pipelines.)
    if (pipes_since_save_ && presents_ - pcache_saved_ > (pcache_saved_ ? 5 * 60 * 60 : 60 * 60)) CacheSavePipelinesSoon();
    ReadQuality();
    if (!dump_dir_.empty()) {
        std::string trig = dump_dir_ + "/step.now";
        if (FILE* f = std::fopen(trig.c_str(), "r")) {
            std::fclose(f);
            std::remove(trig.c_str());
            g_step_frame = int(presents_) + 1;
            ngb_nv2a_logdraws = 1200;   // the NV2A renderer logs its next draws too (NV2ADRAW)
        }
        std::string burst = dump_dir_ + "/burst.now";   // the next 40 frames, every one
        if (FILE* f = std::fopen(burst.c_str(), "r")) {
            std::fclose(f);
            std::remove(burst.c_str());
            dump_from_ = presents_ + 1;
            dump_count_ = 40;
        }
    }
    if (stats_ && presents_ % 60 == 0) {
        std::fprintf(stderr, "[D3D] frame %u: %u draws (%u not assembled, %u no target, %u no pipeline, %u drawn), "
                     "front %08X; draws by target:", presents_, st_.draws, st_.noasm, st_.notarget, st_.nopipe, st_.drawn,
                     front.data);
        for (auto& [k, v] : rt_draws_) std::fprintf(stderr, " %08X:%u", k, v);
        std::fprintf(stderr, "\n");
    }
    st_ = {};
    rt_draws_.clear();
    static uint32_t every = std::getenv("NGB_D3D_DUMP_EVERY") ? uint32_t(std::atoi(std::getenv("NGB_D3D_DUMP_EVERY"))) : 1;
    if (!dump_dir_.empty() && presents_ >= dump_from_ && presents_ < dump_from_ + dump_count_ &&
        (presents_ - dump_from_) % std::max(every, 1u) == 0) {
        Target* t = EnsureTarget(front, false);
        if (t) Dump(*t);
    }
    Image* show = nullptr;
    if (shared_) {
        // the frame for the presenter: a copy, so the next frame's draws into this buffer
        // (the title has two or three in turn) never race the blit to the window
        Target* t = EnsureTarget(front, false);
        if (t) {
            Image& d = display_[size_t(display_next_)];
            if (d.image && (d.w != t->img.w || d.h != t->img.h)) {
                Submit();
                nv2a_vk_queue_lock();
                d_.vkDeviceWaitIdle(dev_);
                nv2a_vk_queue_unlock();
                DestroyImage(d);
            }
            if (!d.image)
                CreateImage(d, kColorFormat, t->img.w, t->img.h, 1, 1, false, false,
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            // the menus' 16:9 look on a wider screen (21:9): black beyond the 16:9 middle -- the game behind a
            // menu too, as at 16:9
            static int mode_log = std::getenv("NGB_MODE_LOG") ? std::atoi(std::getenv("NGB_MODE_LOG")) : 0;
            if (mode_log && (mode_log > 1 || presents_ % 30 == 0))
                std::fprintf(stderr, "[MODE] %u stretched %d screen %d 3d %u (off-screen %u) draws %d paused %d cover %.2f\n",
                             presents_, pk_.stretched, ScreenFrame(threed_), threed_, offscreen3d_,
                             ngb_frame_draws, paused_now_, paused_cover_);
            float bars = 0, bars_y = 0;
            if (pk_.stretched && ScreenFrame(threed_) && t->gw >= 640) {   // (not in play: 3D the screen's shape)
                float km = pk_.hud_shape * 4.0f / 3.0f, ky = 0.75f / pk_.hud_shape;
                if (km < 0.999f) bars = (1 - km) / 2;
                if (ky < 0.999f) bars_y = (1 - ky) / 2;
                // a menu drawn over the paused game (the save statue's) shows the game around it; the menus with
                // their own art over the whole screen (the pause menu's) draw no paused game, and keep the bars
                if (paused_now_) bars = bars_y = 0;
            }
            if (t->gw >= 640) HudGroup(float(t->gw), float(t->gh));
            static int edge_fill = -1;   // NGB_BAR_FILL=0: black bars
            if (edge_fill < 0) edge_fill = !(std::getenv("NGB_BAR_FILL") && *std::getenv("NGB_BAR_FILL") == '0');
            if (!(bars > 0 || bars_y > 0)) edge_frames_ = 0;   // the next 2D screen samples its own
            if (d.image && (gamma_on_ || bars > 0 || bars_y > 0)) {
                EndRendering();
                edge_use_ = edge_fill != 0;
                if (edge_fill && (bars > 0 || bars_y > 0)) EdgeSample(t->img, bars, bars_y);
                GammaCopy(t->img, d, bars, bars_y);
                Barrier(d, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                show = &d;
                display_next_ = (display_next_ + 1) % 3;
            } else if (d.image) {
                EndRendering();
                Barrier(t->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                Barrier(d, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkImageCopy c{};
                c.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                c.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                c.extent = {d.w, d.h, 1};
                d_.vkCmdCopyImage(cmd_, t->img.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, d.image,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
                Barrier(d, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                show = &d;
                display_next_ = (display_next_ + 1) % 3;
            }
        }
    }
    if (show) QuickMenu(*show);
    hud_menu_ = hud_cover_ >= 0.5f && threed_ < 30;
    for (int g = 0; g < 4; g++) {
        gauge_prev_[g][0] = g < gauges_now_ ? gauge_x_[g][0] : 1e30f;
        gauge_prev_[g][1] = g < gauges_now_ ? gauge_x_[g][1] : -1e30f;
    }
    gauges_now_ = 0;
    threed_prev_ = threed_;
    threed_ = 0;
    paused_now_ = false;
    covered_now_ = false;
    paused_cover_ = 0;
    pic_tex_ = 0;
    pic_x1_ = -1;
    offscreen3d_ = 0;
    ngb_frame_draws = ngb_frame_3d_draws = 0;
    hud_cover_ = 0;
    if (trace_) {
        static const char* nopic = std::getenv("NGB_TRACE_NOPIC");
        if (!nopic || (!std::strcmp(nopic, "copy") && presents_ <= trace_pic_until_))
            if (Target* t = EnsureTarget(front, false)) TraceFrame(*t);
        if (--trace_left_ <= 0) {
            std::fclose(trace_);
            trace_ = nullptr;
            std::fprintf(stderr, "[TRACE] done: %s\n", trace_dir_.c_str());
        }
    }
    if (view_seen_) {
        std::fprintf(stderr, "[VIEW] %u yaw %.2f pitch %.2f\n", presents_, view_yaw_, view_pitch_);
        view_seen_ = false;
    }
    EndFrame();
    // after the frame is on the queue: the presenter's blit, later on the same queue, waits for it
    // (its barrier on the picture)
    if (show) nv2a_present_set_external(show->image, show->w, show->h);
    frame_++;
}


// ---- the shader cache ----
//
// Making a shader (HLSL to SPIR-V with glslang) and its pipelines takes milliseconds: the first time a
// draw needs one, the frame waited (a hitch when an effect first shows). So each module's inputs
// (recipes.bin: the vertex program's words, the fixed function or pixel key) and each pipeline's key are
// kept, with the SPIR-V by the hash of its HLSL (spirv.bin), under $NV2A_SHADER_CACHE/d3d; and Vulkan's
// pipeline cache (pipelines.bin). At startup a thread makes them all again before the game needs them --
// from the HLSL the current code generates, so a changed generator only recompiles. A seed shipped with
// the program (share/NinjaGaidenBlack/d3d-seed.bin, NGB_D3D_SEED) holds the same records from test runs,
// the vertex programs as places in the player's own default.xbe: a first start has them all too.
// NGB_D3D_SEED_RECORD=<file> writes such a seed. Records: kind (1), key (8), length (4), data.
//   V vertex program: regs (2) decode (8) quad (1) words (4 n)     W the same, the words at an XBE offset:
//   F fixed function: regs (2) pretransformed (1) decode (8) FixedKey     regs decode quad offset (4) n (4)
//   P pixel: PixelKey                                                     K pipeline: PipeKey

static const char kCacheMagic[8] = {'N', 'G', 'B', 'D', '3', 'D', 'C', '1'};

static std::vector<uint8_t> ReadFile(const std::string& path) {
    std::vector<uint8_t> v;
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
        std::fseek(f, 0, SEEK_END);
        long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        if (n > 0) {
            v.resize(size_t(n));
            if (std::fread(v.data(), 1, v.size(), f) != v.size()) v.clear();
        }
        std::fclose(f);
    }
    return v;
}

// SPIR-V as it was made: its magic, and instructions that end exactly at its end
static bool SpirvWhole(const std::vector<uint32_t>& w) {
    if (w.size() < 5 || w[0] != 0x07230203u) return false;
    size_t p = 5;
    while (p < w.size()) {
        uint32_t words = w[p] >> 16;
        if (!words) return false;
        p += words;
    }
    return p == w.size();
}

static FILE* OpenAppend(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "ab");
    // At the end first: Windows' ftell says 0 for a file just opened to append, and the header went in
    // again at every start -- the readers stopped at it, so nothing past the first session was ever
    // used again (shaders made anew at each launch, draws left out meanwhile: #21).
    if (f) std::fseek(f, 0, SEEK_END);
    if (f && std::ftell(f) == 0) std::fwrite(kCacheMagic, 1, 8, f);
    return f;
}

void VulkanBackend::CacheOpen() {
    if (std::getenv("NGB_D3D_NO_CACHE")) return;
    const char* base = std::getenv("NGB_D3D_CACHE");
    std::string dir;
    if (base) dir = base;
    else if (const char* sc = std::getenv("NV2A_SHADER_CACHE")) dir = std::string(sc) + "/d3d";
    if (!dir.empty()) {
        ngb_mkdir(dir.c_str());
        cache_dir_ = dir;
        // the SPIR-V known, read now (lookups only)
        std::vector<uint8_t> sp = ReadFile(dir + "/spirv.bin");
        if (sp.size() >= 8 && !std::memcmp(sp.data(), kCacheMagic, 8)) {
            size_t o = 8;
            while (o + 12 <= sp.size()) {
                if (!std::memcmp(&sp[o], kCacheMagic, 8)) {   // a header again (written by older Windows builds)
                    o += 8;
                    continue;
                }
                uint64_t h;
                uint32_t n;
                std::memcpy(&h, &sp[o], 8);
                std::memcpy(&n, &sp[o + 8], 4);
                if (o + 12 + size_t(n) * 4 > sp.size()) break;
                std::vector<uint32_t> w(n);
                std::memcpy(w.data(), &sp[o + 12], size_t(n) * 4);
                if (!SpirvWhole(w)) break;
                spirv_by_src_[h] = std::move(w);
                o += 12 + size_t(n) * 4;
            }
            // A record cut short -- the disk full, the game killed while writing -- and the ones appended
            // after it were read as one: another record's bytes as a shader's SPIR-V, and the driver
            // crashed making it at the next start. The file is kept up to the last whole record.
            if (o < sp.size()) {
                std::fprintf(stderr, "[D3D] shader cache: spirv.bin damaged at %zu of %zu bytes, kept up to there\n",
                             o, sp.size());
                std::string tmp = dir + "/spirv.bin.tmp", dst = dir + "/spirv.bin";
                if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
                    bool ok = std::fwrite(sp.data(), 1, o, f) == o;
                    ok = std::fclose(f) == 0 && ok;
#if defined(_WIN32)
                    if (ok) std::remove(dst.c_str());
#endif
                    if (ok) std::rename(tmp.c_str(), dst.c_str());
                    else std::remove(dst.c_str());
                }
            }
        } else if (!sp.empty()) {
            std::remove((dir + "/spirv.bin").c_str());
        }
        std::vector<uint8_t> rec = ReadFile(dir + "/recipes.bin");
        if (!rec.empty() && (rec.size() < 8 || std::memcmp(rec.data(), kCacheMagic, 8))) {
            std::remove((dir + "/recipes.bin").c_str());
        } else if (!rec.empty()) {
            // The same for the recipes: past a record cut short the reading stops, so everything recorded after it
            // -- each launch appending again what it had to make -- was never known, and made late at every start
            // (a draw left out until then: walls and effects popping in). Kept up to the last whole record.
            size_t o = 8;
            while (o + 13 <= rec.size()) {
                if (!std::memcmp(&rec[o], kCacheMagic, 8)) {
                    o += 8;
                    continue;
                }
                uint32_t n;
                std::memcpy(&n, &rec[o + 9], 4);
                if (!std::strchr("VWFPK", char(rec[o])) || !rec[o] || o + 13 + size_t(n) > rec.size()) break;
                o += 13 + size_t(n);
            }
            if (o < rec.size()) {
                std::fprintf(stderr, "[D3D] shader cache: recipes.bin damaged at %zu of %zu bytes, kept up to there\n",
                             o, rec.size());
                std::string tmp = dir + "/recipes.bin.tmp", dst = dir + "/recipes.bin";
                if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
                    bool ok = std::fwrite(rec.data(), 1, o, f) == o;
                    ok = std::fclose(f) == 0 && ok;
#if defined(_WIN32)
                    if (ok) std::remove(dst.c_str());
#endif
                    if (ok) std::rename(tmp.c_str(), dst.c_str());
                    else std::remove(dst.c_str());
                }
            }
        }
        recipes_ = OpenAppend(dir + "/recipes.bin");
        spirv_ = OpenAppend(dir + "/spirv.bin");
    }
    // Vulkan's pipeline cache, with what was saved
    std::vector<uint8_t> pc = dir.empty() ? std::vector<uint8_t>() : ReadFile(dir + "/pipelines.bin");
    VkPipelineCacheCreateInfo pci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    pci.initialDataSize = pc.size();
    pci.pInitialData = pc.empty() ? nullptr : pc.data();
    if (d_.vkCreatePipelineCache(dev_, &pci, nullptr, &pcache_) != VK_SUCCESS) {
        pci.initialDataSize = 0;
        pci.pInitialData = nullptr;
        d_.vkCreatePipelineCache(dev_, &pci, nullptr, &pcache_);
    }
    if (const char* sr = std::getenv("NGB_D3D_SEED_RECORD")) seed_out_ = OpenAppend(sr);
    if (!std::getenv("NGB_D3D_NO_PREWARM")) prewarm_ = std::thread([this] { Prewarm(); });
    async_ = !std::getenv("NGB_D3D_SYNC_COMPILE");
    if (const char* b = std::getenv("NGB_D3D_COMPILE_BUDGET_MS")) compile_budget_us_ = uint64_t(std::max(0.0, std::atof(b)) * 1000);
    if (async_) compiler_ = std::thread([this] { Compiler(); });
}

void VulkanBackend::CacheRecord(char kind, uint64_t key, const void* data, size_t n, FILE* only) {
    std::lock_guard<std::recursive_mutex> g(cache_mu_);
    auto put = [&](FILE* f, char k, const void* d, uint32_t len) {
        if (!f) return;
        std::fwrite(&k, 1, 1, f);
        std::fwrite(&key, 8, 1, f);
        std::fwrite(&len, 4, 1, f);
        std::fwrite(d, 1, len, f);
        std::fflush(f);
    };
    if (only) {
        put(only, kind, data, uint32_t(n));
        return;
    }
    put(recipes_, kind, data, uint32_t(n));
    if (!seed_out_) return;
    if (kind != 'V') {
        put(seed_out_, kind, data, uint32_t(n));
        return;
    }
    // the seed keeps no program, only where it is in the player's default.xbe
    static std::vector<uint8_t> xbe;
    if (xbe.empty())
        if (const char* xp = std::getenv("NV2A_SHADER_SEED_XBE")) xbe = ReadFile(xp);
    const uint8_t* d = static_cast<const uint8_t*>(data);
    uint32_t words;
    std::memcpy(&words, d + 11, 4);
    const uint8_t* prog = d + 15;
    if (xbe.empty() || !words) return;
    auto it = std::search(xbe.begin(), xbe.end(), prog, prog + size_t(words) * 4);
    if (it == xbe.end()) return;
    uint8_t w[19];
    std::memcpy(w, d, 11);
    uint32_t off = uint32_t(it - xbe.begin());
    std::memcpy(w + 11, &off, 4);
    std::memcpy(w + 15, &words, 4);
    put(seed_out_, 'W', w, sizeof(w));
}

std::string VulkanBackend::CurrentVertexProgramRecipe(const std::vector<uint32_t>& prog, uint16_t regs, uint64_t decode,
                                                      uint32_t quad, std::vector<uint8_t>& out) {
    uint32_t n = uint32_t(prog.size());
    out.resize(15 + size_t(n) * 4);
    std::memcpy(&out[0], &regs, 2);
    std::memcpy(&out[2], &decode, 8);
    out[10] = uint8_t(quad);
    std::memcpy(&out[11], &n, 4);
    std::memcpy(&out[15], prog.data(), size_t(n) * 4);
    return {};
}

// The periodic save, on a thread of its own. Made on the frame it stalled that frame: the driver's
// cache data is tens of MB on Nvidia (a few hundred KB on RADV), copied out and written while the game
// waited -- a hitch every 10-20 s in play, with the frame counter still saying 60 and the audio
// crackling (issue #17). A Vulkan pipeline cache may be read while pipelines are made with it.
void VulkanBackend::CacheSavePipelinesSoon() {
    if (saving_.exchange(true)) return;   // the last one still writing: next frame, then
    if (saver_.joinable()) saver_.join();
    pcache_saved_ = presents_;
    pipes_since_save_ = 0;
    saver_ = std::thread([this] {
        CacheWritePipelines();
        saving_ = false;
    });
}

void VulkanBackend::CacheSavePipelines() {
    pcache_saved_ = presents_;
    pipes_since_save_ = 0;
    CacheWritePipelines();
}

void VulkanBackend::CacheWritePipelines() {
    if (cache_dir_.empty() || !pcache_) return;
    size_t n = 0;
    if (d_.vkGetPipelineCacheData(dev_, pcache_, &n, nullptr) != VK_SUCCESS || !n) return;
    std::vector<uint8_t> v(n);
    if (d_.vkGetPipelineCacheData(dev_, pcache_, &n, v.data()) != VK_SUCCESS) return;
    std::string tmp = cache_dir_ + "/pipelines.bin.tmp";
    if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
        bool ok = std::fwrite(v.data(), 1, n, f) == n;
        ok = std::fclose(f) == 0 && ok;
        if (ok) {
            std::string dst = cache_dir_ + "/pipelines.bin";
#if defined(_WIN32)
            std::remove(dst.c_str());   // Windows' rename does not replace: the cache was never updated there
#endif
            std::rename(tmp.c_str(), dst.c_str());
        }
    }
}

// The prewarm thread: every module and pipeline the cache and the seed know, made before they are needed.
// The thread making what draws asked for (Module and Pipeline while async_now_), in order.
void VulkanBackend::Compiler() {
    for (;;) {
        Job j;
        {
            std::unique_lock<std::mutex> g(job_mu_);
            job_cv_.wait(g, [&] { return stop_ || !jobs_.empty(); });
            if (stop_) return;
            j = std::move(jobs_.front());
            jobs_.pop_front();
        }
        if (!j.pipeline) {
            std::vector<uint32_t> spirv;
            VkShaderModule m = Compile(j.hlsl, j.vertex, spirv) ? MakeModule(spirv) : VK_NULL_HANDLE;
            {
                std::lock_guard<std::recursive_mutex> g(cache_mu_);
                if (modules_.count(j.key)) {
                    if (m) d_.vkDestroyShaderModule(dev_, m, nullptr);
                } else {
                    modules_[j.key] = m;
                }
            }
            std::lock_guard<std::mutex> g(job_mu_);
            pending_mods_.erase(j.key);
        } else {
            VkShaderModule vs, ps;
            {
                std::lock_guard<std::recursive_mutex> g(cache_mu_);
                vs = modules_[j.pk.vs];
                ps = modules_[j.pk.ps];
            }
            VkPipeline p = vs && ps ? MakePipeline(j.pk, vs, ps) : VK_NULL_HANDLE;
            bool made = false;
            {
                std::lock_guard<std::recursive_mutex> g(cache_mu_);
                if (pipelines_.count(j.pk)) {
                    if (p) d_.vkDestroyPipeline(dev_, p, nullptr);
                } else {
                    pipelines_[j.pk] = p;      // a failed one too: the draw is not asked for again
                    made = p != VK_NULL_HANDLE;
                }
            }
            if (made) {
                CacheRecord('K', 0, &j.pk, sizeof(j.pk));
                pipes_since_save_++;
            }
            std::lock_guard<std::mutex> g(job_mu_);
            pending_pipes_.erase(j.pk);
        }
        async_made_++;
    }
}

void VulkanBackend::Prewarm() {
    struct Rec { char kind; uint64_t key; std::vector<uint8_t> data; };
    std::vector<Rec> recs;
    auto parse = [&](const std::vector<uint8_t>& b) {
        if (b.size() < 8 || std::memcmp(b.data(), kCacheMagic, 8)) return;
        size_t o = 8;
        while (o + 13 <= b.size()) {
            if (!std::memcmp(&b[o], kCacheMagic, 8)) {   // a header again (written by older Windows builds)
                o += 8;
                continue;
            }
            Rec r;
            r.kind = char(b[o]);
            std::memcpy(&r.key, &b[o + 1], 8);
            uint32_t n;
            std::memcpy(&n, &b[o + 9], 4);
            if (o + 13 + n > b.size()) break;
            r.data.assign(b.begin() + long(o + 13), b.begin() + long(o + 13 + n));
            recs.push_back(std::move(r));
            o += 13 + n;
        }
    };
    std::string seed;
    if (const char* e = std::getenv("NGB_D3D_SEED")) seed = e;
    else {
        char exe[4096];
        if (ngb_exe_path(exe, sizeof exe) > 0) {
            if (char* sl = std::strrchr(exe, '/')) {
                *sl = 0;
                seed = std::string(exe) + "/../share/NinjaGaidenBlack/d3d-seed.bin";
                if (FILE* f = std::fopen(seed.c_str(), "rb")) std::fclose(f);   // (Windows: beside the .exe)
                else seed = std::string(exe) + "/share/NinjaGaidenBlack/d3d-seed.bin";
            }
        }
    }
    if (!seed.empty()) parse(ReadFile(seed));
    size_t from_seed = recs.size();
    if (!cache_dir_.empty()) parse(ReadFile(cache_dir_ + "/recipes.bin"));
    // the seed's pipelines already in recipes.bin: not copied there again (it grew by the whole seed at
    // every launch)
    std::unordered_map<PipeKey, bool, PipeHash> recorded;
    for (size_t i = from_seed; i < recs.size(); i++)
        if (recs[i].kind == 'K' && recs[i].data.size() == sizeof(PipeKey)) {
            PipeKey k;
            std::memcpy(&k, recs[i].data.data(), sizeof(k));
            recorded[k] = true;
        }
    {   // what is known, before any of it is made: a draw needing one makes it at once (Pipeline)
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        for (const Rec& r : recs)
            if (r.kind == 'K' && r.data.size() == sizeof(PipeKey)) {
                PipeKey k;
                std::memcpy(&k, r.data.data(), sizeof(k));
                known_pipes_[k] = true;
            }
    }
    std::vector<uint8_t> xbe;
    auto t0 = std::chrono::steady_clock::now();
    uint32_t made = 0, pipes = 0;
    for (int pass = 0; pass < 2; pass++)   // the modules, then the pipelines
        for (const Rec& r : recs) {
            if (stop_) return;
            if ((r.kind == 'K') != (pass == 1)) continue;
            if (r.kind == 'K') {
                if (r.data.size() != sizeof(PipeKey)) continue;
                PipeKey k;
                std::memcpy(&k, r.data.data(), sizeof(k));
                VkShaderModule vs, ps;
                {
                    std::lock_guard<std::recursive_mutex> g(cache_mu_);
                    if (pipelines_.count(k)) continue;
                    auto a = modules_.find(k.vs), b = modules_.find(k.ps);
                    if (a == modules_.end() || b == modules_.end() || !a->second || !b->second) continue;
                    vs = a->second;
                    ps = b->second;
                }
                VkPipeline p = MakePipeline(k, vs, ps);
                std::lock_guard<std::recursive_mutex> g(cache_mu_);
                if (pipelines_.count(k)) {
                    if (p) d_.vkDestroyPipeline(dev_, p, nullptr);
                } else {
                    pipelines_[k] = p;
                    pipes++;
                    if (&r - recs.data() < long(from_seed) && !recorded.count(k))
                        CacheRecord('K', 0, r.data.data(), r.data.size(), recipes_);
                }
                continue;
            }
            {
                std::lock_guard<std::recursive_mutex> g(cache_mu_);
                if (modules_.count(r.key)) continue;
            }
            std::string hlsl;
            bool vertex = true;
            const uint8_t* d = r.data.data();
            if ((r.kind == 'V' && r.data.size() >= 15) || (r.kind == 'W' && r.data.size() == 19)) {
                uint16_t regs;
                uint64_t decode;
                uint32_t n;
                std::memcpy(&regs, d, 2);
                std::memcpy(&decode, d + 2, 8);
                std::vector<uint32_t> words;
                if (r.kind == 'V') {
                    std::memcpy(&n, d + 11, 4);
                    if (r.data.size() != 15 + size_t(n) * 4) continue;
                    words.resize(n);
                    std::memcpy(words.data(), d + 15, size_t(n) * 4);
                } else {
                    uint32_t off;
                    std::memcpy(&off, d + 11, 4);
                    std::memcpy(&n, d + 15, 4);
                    if (xbe.empty())
                        if (const char* xp = std::getenv("NV2A_SHADER_SEED_XBE")) xbe = ReadFile(xp);
                    if (size_t(off) + size_t(n) * 4 > xbe.size()) continue;
                    words.resize(n);
                    std::memcpy(words.data(), &xbe[off], size_t(n) * 4);
                }
                // the key as Draw makes it: a different copy of the game makes no match, and is skipped
                uint64_t key = Hash(&decode, 8, (Hash(words.data(), words.size() * 4) | 1) ^ regs);
                if (d[10]) key ^= 0xC3C30000;
                if (key != r.key) continue;
                hlsl = VertexProgramHlsl(words, regs, decode);
                if (d[10]) hlsl = QuadHlsl(std::move(hlsl));   // as Draw makes a screen quad's
            } else if (r.kind == 'F' && r.data.size() == 11 + sizeof(FixedKey)) {
                uint16_t regs;
                uint64_t decode;
                FixedKey fk;
                std::memcpy(&regs, d, 2);
                std::memcpy(&decode, d + 3, 8);
                std::memcpy(&fk, d + 11, sizeof(fk));
                hlsl = FixedVertexHlsl(regs, d[2] != 0, decode, fk);
                if (d[2]) hlsl = QuadHlsl(std::move(hlsl));    // pretransformed: as Draw makes it
            } else if (r.kind == 'P' && r.data.size() == sizeof(PixelKey)) {
                PixelKey pk;
                std::memcpy(&pk, d, sizeof(pk));
                hlsl = PixelHlsl(pk);
                vertex = false;
            } else {
                continue;
            }
            std::vector<uint32_t> spirv;
            VkShaderModule m = Compile(hlsl, vertex, spirv) ? MakeModule(spirv) : VK_NULL_HANDLE;
            std::lock_guard<std::recursive_mutex> g(cache_mu_);
            if (modules_.count(r.key)) {
                if (m) d_.vkDestroyShaderModule(dev_, m, nullptr);
            } else {
                modules_[r.key] = m;
                made++;
            }
        }
    std::fprintf(stderr, "[D3D] shader cache: %u shaders and %u pipelines made ahead in %.1f s\n", made, pipes,
                 std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    // the pipelines again for the anti-aliasing in use: each was kept with the sample count it was made
    // for (the shipped seed's without MSAA), and a pipeline is one sample count's only
    int sc = 1, ms = 1;
    nv2a_quality_get(&sc, &sc, &sc, &ms);
    while (ms > 1 && !(ms_counts_ & uint32_t(ms))) ms /= 2;
    warm_want_ = uint32_t(ms);
    WarmSamples(uint32_t(ms));
    prewarm_done_ = 1;
}

// Every pipeline made so far again for a sample count (anti-aliasing), in the background: a change of
// the setting otherwise made each again as the draws first needed it.
void VulkanBackend::WarmSamples(uint32_t samples) {
    std::vector<PipeKey> keys;
    {
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        for (auto& [k, p] : pipelines_)
            if (k.samples != samples && (k.samples > 1 || samples > 1)) {
                PipeKey v = k;
                v.samples = samples;
                if (!pipelines_.count(v)) keys.push_back(v);
            }
    }
    auto t0 = std::chrono::steady_clock::now();
    uint32_t n = 0;
    for (const PipeKey& k : keys) {
        if (stop_ || warm_want_ != samples) return;
        VkShaderModule vs, ps;
        {
            std::lock_guard<std::recursive_mutex> g(cache_mu_);
            if (pipelines_.count(k)) continue;
            auto a = modules_.find(k.vs), b = modules_.find(k.ps);
            if (a == modules_.end() || b == modules_.end() || !a->second || !b->second) continue;
            vs = a->second;
            ps = b->second;
        }
        VkPipeline p = MakePipeline(k, vs, ps);
        std::lock_guard<std::recursive_mutex> g(cache_mu_);
        if (pipelines_.count(k)) {
            if (p) d_.vkDestroyPipeline(dev_, p, nullptr);
        } else {
            pipelines_[k] = p;
            n++;
        }
    }
    if (n)
        std::fprintf(stderr, "[D3D] shader cache: %u pipelines made for %ux anti-aliasing in %.1f s\n", n, samples,
                     std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
}


}  // namespace

std::unique_ptr<Backend> CreateBackend() { return std::make_unique<VulkanBackend>(); }

}  // namespace gpu
