#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "gpu/state.h"

// HLSL for the title's shaders and for its fixed function state, per the XDK's definitions of both
namespace gpu {

bool IdView();
int IdViewMode();   // NGB_D3D_IDVIEW: pixel shaders tint opaque 3D draws by their colour (debugging)

enum TexKind : uint8_t { kTexNone, kTex2D, kTex3D, kTexCube };

struct TexInfo {
    uint8_t kind = kTexNone;
    uint8_t rect = 0;    // linear texture, coordinates in texels
    uint8_t shadow = 0;  // depth texture: 1 D24, 2 D16
    uint8_t flags = 0;   // kTexWide; bits 0 and 1 are a backend extension's
};
enum : uint8_t {
    kTexWide = 4,  // an upscaled target shrunk by the pass: each tap averaged over console texels
    kTexSigned = 8,  // a bump map (V8U8): its du dv decoded texel by texel in the shader (bumpduv)
    kTexLinear = 16, // a screen quad's texture filtered linearly: weighted by alpha above 1x (QuadSnap, #83)
};

struct PixelKey {
    uint32_t programmable = 0;
    // combiners, straight from D3DPIXELSHADERDEF
    uint32_t count = 0, rgb_in[8] = {}, rgb_out[8] = {}, a_in[8] = {}, a_out[8] = {}, final_abcd = 0, final_efg = 0;
    uint32_t tex_modes = 0, dot_mapping = 0, input_texture = 0, compare_mode = 0;
    // fixed function stages: colorop, arg1, arg2, arg0, alphaop, arg1, arg2, arg0, result
    uint32_t ff[4][9] = {};
    TexInfo tex[4];
    uint32_t alphakill = 0;  // bit per stage; bits 4-7: the stage's 2D read left out past 0..1 (the heat haze, #55)
    uint32_t flat = 0, two_sided = 0;
    uint32_t proj = 0;       // fixed function: bit per stage sampled at coordinates divided by w
    // a screen quad (QuadHlsl); a point sprite, stage 3 at the point's own coordinates after the stages before it
    // (#51). Halves of what was one uint32_t quad: the key's size and bytes as the shader cache has them.
    uint16_t quad = 0, sprite = 0;
    bool operator==(const PixelKey& o) const;
};

uint64_t Hash(const void* p, size_t n, uint64_t h = 1469598103934665603ull);

PixelKey MakePixelKey(const State& s, const TexInfo tex[4]);

// vertex program microcode to HLSL, regs is the bit mask of v registers the draw supplies
// decode: VertexDecode per register, 4 bits each, for elements the input assembler hands over raw
std::string VertexProgramHlsl(const std::vector<uint32_t>& program, uint16_t regs, uint64_t decode = 0);
// fixed function vertex state a shader is built for
struct FixedKey {
    uint32_t light = 0;    // bit 0 lighting, 1 specular, 2 local viewer, 3 normalize, then D3DMCS 2 bits each from bit 8:
                           // diffuse, specular, ambient, emissive (material when the vertex lacks that colour)
    uint32_t types = 0;    // D3DLIGHTTYPE per light, 2 bits each, 0 off
    uint32_t tex[4] = {};  // per stage: coordinate set, texgen at bit 4, D3DTTFF count at bit 8, projected bit 11, set size at bit 12
    uint32_t blend = 0;    // D3DVERTEXBLENDFLAGS (the NV2A skin mode): 1, 3, 5 n weights for n + 1 matrices, the last
                           // 1 - the others; 2, 4, 6 a weight for each of 2, 3, 4 matrices
};
FixedKey MakeFixedKey(const State& s);
std::string FixedVertexHlsl(uint16_t regs, bool pretransformed, uint64_t decode = 0, const FixedKey& f = {});
std::string PixelHlsl(const PixelKey& k);
// a screen quad's vertex or pixel shader: texture coordinates as the quad was drawn, before its far edges were pulled out
std::string QuadHlsl(std::string s);
std::string PointHlsl(std::string s, int kind);
std::string SpriteHlsl(std::string s);

// HLSL a backend extension adds; the defaults write the console path
struct ShaderHooks {
    virtual ~ShaderHooks() = default;
    // the cbuffer line for DrawConstants::ext
    virtual std::string Constants() const;
    // float4 shadow%d(float2 uv, float z, int fmt, float2 px, float2 size): stage i's texture compared through its
    // sampler (a helper per stage, naming them: see PixelHlsl)
    virtual std::string Shadow(int i) const;
    // what spreadtap scales a render target's colour by
    virtual std::string ReadScale(const TexInfo&) const { return "1"; }
    // a cube sampled along dir at stage i, empty for a plain sample
    virtual std::string Cube(const TexInfo&, int, const std::string&) const { return {}; }
    // after stage i's fetch
    virtual std::string Fetched(const TexInfo&, int) const { return {}; }
    // outc to the target, after the alpha test
    virtual std::string Output() const;
};
inline const ShaderHooks* g_shader_hooks = nullptr;

// the constant buffer layout both stages share, register b1
struct DrawConstants {
    float surf[4];      // target width, height, 1 / depth scale, 0
    float fog[4];       // start, end, density, mode
    float fog_color[4];
    float misc[4];      // alpha ref, alpha func, shadow func, specular
    float pc[18][4];    // combiner c0 c1 per stage, then final c0 c1
    float tfactor[4];
    float texsize[4][4];
    float bump[4][4];   // 00 01 10 11
    float bumpl[4][4];  // scale offset, and for bumpduv the stage's LOD bias and most detailed level
    float wvp[16];      // fixed function
    float ext[4];       // a backend extension's, declared by ShaderHooks::Constants
    float spread[4][4]; // kTexWide: tap step in uv, taps per axis
    // fixed function, lit in view space
    float wv[16];
    float wvn[16];       // normals: inverse transpose of wv
    float tm[4][16];     // texture transforms
    float mat[5][4];     // material diffuse, ambient, specular, emissive, power
    float amb[4];        // D3DRS_AMBIENT
    float lt[8][7][4];   // per light: diffuse, specular, ambient, position range, direction falloff, attenuation, cos theta/2 phi/2
    // vertex blending: world n x view for world matrices 0-3, their normal matrices, and projection x viewport
    float wvb[4][16];
    float wvbn[4][16];
    float pv[16];
    // a 2D quad above 1x (backend QuadSnap): each stage's texture coordinates kept to what the console's pixel
    // centres sample (min xy, max zw, in the vertices' own units); their gradients du/dx du/dy dv/dx dv/dy in
    // console pixels; qsnap.x 1 when they apply. Other draws: qsnap.x 0.
    float uvc[4][4];
    float uvg[4][4];
    float qsnap[4];
};

}  // namespace gpu
