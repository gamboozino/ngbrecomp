// RECOMP SETTINGS -> Graphics -> Bloom (ngb_pc_setting 18): 2 On (the game's passes, fed and finished at the console's
// size: steady at any render scale, #248; the default, #270), 3 Off (the glow not added). 0 Original (the game's passes
// as they are) and 1 Reduced (the continuous chain below, fainter: #64) are no longer offered: NGB_BLOOM=0..3 overrides
// the setting, and NGB_GLOW_QUALITY=1 or 0 as before (Reduced or Original), for comparing.
inline int GlowMode() {
    static int forced=[] {
        const char* e=std::getenv("NGB_BLOOM");
        if (e && *e) return std::clamp(std::atoi(e),0,3);
        e=std::getenv("NGB_GLOW_QUALITY");
        return e && *e ? std::strcmp(e,"0") ? 1 : 0 : -1;
    }();
    int v=std::clamp(ngb_pc_setting(18),0,3);
    return forced >= 0 ? forced : v==1 ? 2 : v;
}
inline bool ContinuousGlowEnabled() { return GlowMode()==1; }
inline bool GlowCorrected() { return GlowMode()==2; }
inline bool GlowOff() { return GlowMode()==3; }
inline int GlowSoftenMode() {
    static int mode=[] {const char* e=std::getenv("NGB_GLOW_SOFTEN");return e?std::clamp(std::atoi(e),0,2):0;}();
    return mode;
}
inline float GlowStrength() {
    static float strength=[] {
        const char* value=std::getenv("NGB_GLOW_STRENGTH");
        if (!value || !*value) return 0.25f;
        char* end=nullptr;float parsed=std::strtof(value,&end);
        if (end==value || *end || !std::isfinite(parsed)) return 0.25f;
        return std::clamp(parsed,0.0f,1.0f);
    }();
    return strength;
}

inline std::string GlowStrengthHlsl(std::string src) {
    const std::string needle="return outc;";
    size_t at=src.find(needle);
    if (at==std::string::npos) return src;   // not the expected shader: drawn at full strength
    // Scale the completed glow RGB, after the original combiner clamp and
    // before its additive blend. Alpha, the scene and the blur stay intact.
    src.replace(at,needle.size(),"outc.rgb *= bumpmat[3].z;\n    return outc;");
    return src;
}

inline bool GlowQuad(const Assembled& a, uint32_t count, uint32_t w, uint32_t h) {
    if (!a.pretransformed || count!=4 || !(a.regs&1)) return false;
    const auto& e=a.elements[0];
    if (e.format!=2 || e.buffer>=a.buffer_count) return false;
    const auto& b=a.buffers[e.buffer];
    if (!b.src || !b.stride || size_t(e.offset)+size_t(b.stride)*3+8>b.bytes) return false;
    float x0=1e10f,y0=1e10f,x1=-1e10f,y1=-1e10f;
    for (int i=0;i<4;++i) {
        float xy[2]; std::memcpy(xy,b.src+size_t(i)*b.stride+e.offset,8);
        if (!std::isfinite(xy[0]) || !std::isfinite(xy[1])) return false;
        x0=std::min(x0,xy[0]); y0=std::min(y0,xy[1]);
        x1=std::max(x1,xy[0]); y1=std::max(y1,xy[1]);
    }
    return std::abs(x0)<=0.5f && std::abs(y0)<=0.5f &&
        std::abs(x1-float(w))<=0.5f && std::abs(y1-float(h))<=0.5f;
}

inline uint32_t GlowTargetScale(uint32_t ordinary) {
    return std::max(2u,std::min(ordinary,8192u/128u));
}

// Native extraction texels. Keep displayed width relative to picture height;
// the configured picture aspect includes letterboxing and ignores HUD stretch.
struct GlowFootprintValues { double x=1,y=8,aspect=4.0/3.0; };
inline GlowFootprintValues GlowFootprint() {
    GlowFootprintValues result;
    switch (ngb_pc_setting(7)) {
    case 1: result.aspect=16.0/9.0;break;
    case 2: result.aspect=16.0/10.0;break;
    case 3: result.aspect=ngb_aspect_21_9();break;
    case 4: result.aspect=32.0/9.0;break;
    default: break;
    }
    result.x*= (16.0/9.0)/result.aspect;
    return result;
}
