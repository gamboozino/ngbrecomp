// Aspect correction for untextured two-vertex impact streaks (#65, HammerD72; NGB_SPARK_ASPECT_FIX=0: off).
// Rasterize their stroke in a 16:9 pixel metric, then compensate its X
// footprint for ultrawide presentation. Endpoints stay in the original WVP.
inline double SparkAspect() {
    switch (ngb_pc_setting(7)) {
    case 1: return 16.0/9.0;
    case 2: return 16.0/10.0;
    case 3: return 64.0/27.0;
    default: return 4.0/3.0;
    }
}
inline bool SparkFixEnabled() {
    static const bool enabled=[] {const char* e=std::getenv("NGB_SPARK_ASPECT_FIX");return !(e && !std::strcmp(e,"0"));}();
    return enabled;
}
// Spark Thickness (Graphics): a multiplier on the weapon impact-spark stroke width
// (the wide lines the game draws at 2.7 to 3 pixels). 0 = 1/3, 1 = 1/2, 2 = 2/3,
// 3 = the game's own (1x). The width scales by this at every aspect and render
// scale, so the sparks can be made thinner.
inline float SparkThickness() {
    switch (ngb_pc_setting(31)) {
    case 0: return 1.0f/3.0f;
    case 1: return 1.0f/2.0f;
    case 2: return 2.0f/3.0f;
    default: return 1.0f;
    }
}
struct SparkEndpoint { float p[4],color[4]; };
inline bool ReadSparkVertices(const Assembled& a,SparkEndpoint (&out)[2]) {
    if (a.topology!=Topology::Lines || a.pretransformed || a.regs!=9 || a.index_count || a.draw_count!=2) return false;
    const auto& pe=a.elements[0]; const auto& ce=a.elements[3];
    if ((pe.format!=6 && pe.format!=2) || (ce.format!=87 && ce.format!=2) ||
        pe.buffer>=a.buffer_count || ce.buffer>=a.buffer_count) return false;
    const auto& pb=a.buffers[pe.buffer];const auto& cb=a.buffers[ce.buffer];
    const size_t pbytes=pe.format==6?12:16,cbytes=ce.format==87?4:16;
    if (!pb.src || !cb.src || !pb.stride || !cb.stride ||
        size_t(pe.offset)+pb.stride+pbytes>pb.bytes || size_t(ce.offset)+cb.stride+cbytes>cb.bytes) return false;
    for (int v=0;v<2;++v) {
        out[v].p[0]=out[v].p[1]=out[v].p[2]=0;out[v].p[3]=1;
        std::memcpy(out[v].p,pb.src+pe.offset+v*pb.stride,pbytes);
        for (float p:out[v].p) if (!std::isfinite(p)) return false;
        const uint8_t* c=cb.src+ce.offset+v*cb.stride;
        if (ce.format==87) {uint32_t packed;std::memcpy(&packed,c,4);Color(packed,out[v].color);}
        else std::memcpy(out[v].color,c,16);
    }
    return true;
}
inline bool SparkEndpoints(const State& s,uint32_t prim,uint32_t count,const Assembled& a,SparkEndpoint (&out)[2]) {
    if ((prim!=2 && prim!=4) || count!=2 || s.vs_handle!=0x42 || s.ps ||
        s.rs[kRsLighting] || s.rs[kRsVertexBlend] || !s.rs[kRsZEnable] ||
        !SimpleState(s,kMBlendEnable) || SimpleState(s,kMSrcBlend)!=0x302 ||
        SimpleState(s,kMDestBlend)!=1 || SimpleState(s,kMBlendOp)!=0x8006 ||
        s.tss[0][kTssColorOp]!=2 || s.tss[0][kTssColorArg1]!=0 ||
        s.tss[0][kTssAlphaOp]!=2 || s.tss[0][kTssAlphaArg1]!=0 || s.tss[1][kTssColorOp]!=1 ||
        !std::isfinite(Bits(s.rs[kRsLineWidth])) || Bits(s.rs[kRsLineWidth])<=1) return false;
    // The effective fixed-function chain selects diffuse only and then ends.
    // Ignore stale bound textures in inactive stages, just as that chain does.
    return ReadSparkVertices(a,out);
}
struct SparkQuadVertex { float p[4],color[4],other[4],stroke[4]; };
inline bool ExpandSpark(const State& s,const SparkEndpoint (&ends)[2],float aspect_ratio,float width,
                        SparkQuadVertex (&vertices)[6],Assembled& out) {
    if (aspect_ratio<=1.0001f) return false;
    float wv[16],wvp[16],clip[2][4]{};
    MatMul(s.transforms[256].data(),s.transforms[2].data(),wv);
    MatMul(wv,s.transforms[3].data(),wvp);
    for (int v=0;v<2;++v) for (int j=0;j<4;++j)
        clip[v][j]=ends[v].p[0]*wvp[j]+ends[v].p[1]*wvp[4+j]+ends[v].p[2]*wvp[8+j]+wvp[12+j];
    // Use the original line path at the eye plane, for non-finite transforms,
    // and for degenerate segments; do not divide by a zero/negative clip W.
    for (auto& p:clip) {for (float f:p) if (!std::isfinite(f)) return false;if (p[3]<=1e-5f) return false;}
    double dx=(clip[1][0]/clip[1][3]-clip[0][0]/clip[0][3])*s.viewport.w*aspect_ratio;
    double dy=(clip[1][1]/clip[1][3]-clip[0][1]/clip[0][3])*s.viewport.h;
    if (dx*dx+dy*dy<1e-10) return false;
    const int endpoint[6]={0,1,0,0,1,1};
    const float side[6]={-1,-1,1,1,-1,1};
    for (int i=0;i<6;++i) {
        int v=endpoint[i];
        std::memcpy(vertices[i].p,ends[v].p,16);std::memcpy(vertices[i].color,ends[v].color,16);
        std::memcpy(vertices[i].other,ends[1-v].p,16);
        vertices[i].stroke[0]=side[i];vertices[i].stroke[1]=v?-1.f:1.f;
        vertices[i].stroke[2]=aspect_ratio;vertices[i].stroke[3]=width;
    }
    out.regs=(1<<0)|(1<<3)|(1<<9)|(1<<10);out.vertex_count=out.draw_count=6;
    out.topology=Topology::Triangles;out.buffer_count=1;
    out.buffers[0]={reinterpret_cast<const uint8_t*>(vertices),sizeof(vertices),sizeof(SparkQuadVertex)};
    out.elements[0]={2,0,0};out.elements[3]={2,0,16};out.elements[9]={2,0,32};out.elements[10]={2,0,48};
    return true;
}
inline std::string SparkStrokeHlsl(std::string source) {
    const std::string anchor="    o.pos = cp;";
    auto at=source.find(anchor);if (at==std::string::npos) return {};
    source.insert(at,R"(
    // Opposite endpoint and stroke metadata exist only on this private layout.
    float4 opposite = mul(float4(i.v9.xyz, 1), wvp);
    float2 delta = (opposite.xy / opposite.w - cp.xy / cp.w) * float2(surf.x, -surf.y) * 0.5;
    delta.x *= i.v10.z;
    delta *= i.v10.y;
    float2 normal = float2(-delta.y, delta.x) * rsqrt(max(dot(delta, delta), 1e-20));
    normal.x /= i.v10.z;
    float2 stroke = normal * (0.5 * i.v10.w * i.v10.x);
    cp.xy += stroke * float2(2 / surf.x, -2 / surf.y) * cp.w;
)");
    return source;
}
