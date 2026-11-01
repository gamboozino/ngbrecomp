// Filter only the glow image, before its original tint and additive blend.
// Radius is measured in native glow texels, independent of render scale.
bool VulkanBackend::SoftenGlow(Image& source) {
    EndRendering();
    const uint32_t width=source.w,height=source.h;
    for (auto& image:glow_soft_) {
        if (image.image && (image.w!=width || image.h!=height)) Retire(image);
        if (!image.image && !CreateImage(image,kColorFormat,width,height,1,1,false,false,
            VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) return false;
    }
    if (!glow_soft_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter=ci.minFilter=VK_FILTER_LINEAR;
        ci.addressModeU=ci.addressModeV=ci.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (d_.vkCreateSampler(dev_,&ci,nullptr,&glow_soft_sampler_)!=VK_SUCCESS) return false;
    }
    static const uint64_t vs=0x674C4F57534F4656ull;
    if (!Module(vs,[] { return std::string(
        "struct VO { float4 pos:SV_Position; float2 uv:TEXCOORD0; };\n"
        "VO main(uint id:SV_VertexID) { VO o; float2 p=float2((id<<1)&2,id&2);\n"
        "o.uv=p; o.pos=float4(p*2-1,0,1); return o; }\n"); },true)) return false;
    Image* input=&source;
    for (int axis=0;axis<2;++axis) {
        Image& output=glow_soft_[axis];
        auto footprint=GlowFootprint();
        uint32_t options[]={width,height,uint32_t(GlowSoftenMode()),uint32_t(axis),5u,uint32_t(ngb_pc_setting(7))};
        uint64_t ps=Hash(options,sizeof(options),0x674C4F57534F4650ull);
        if (!Module(ps,[&] {
            double sigma=(GlowSoftenMode()==1?0.6:1.2)*double(axis?height:width)/128.0;
            if (!axis) sigma*= (16.0/9.0)/footprint.aspect/8;
            sigma=std::max(sigma,0.3);
            int radius=int(std::ceil(sigma*3));
            double weight_sum=0;
            for (int i=-radius;i<=radius;++i) weight_sum+=std::exp(-double(i*i)/(2*sigma*sigma));
            std::ostringstream code; code<<std::setprecision(12);
            code<<"Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                <<"struct VO { float4 pos:SV_Position; float2 uv:TEXCOORD0; };\n"
                <<"float4 main(VO i):SV_Target { float4 sum=0;\n";
            for (int i=-radius;i<=radius;++i) {
                double offset=double(i)/double(axis?height:width);
                double weight=std::exp(-double(i*i)/(2*sigma*sigma))/weight_sum;
                code<<"sum+=tex0.SampleLevel(smp0,i.uv+float2("<<(axis?0:offset)<<","<<(axis?offset:0)
                    <<"),0)*"<<weight<<";\n";
            }
            code<<"return sum; }\n"; return code.str();
        },false)) return false;
        PipeKey key{};key.vs=vs;key.ps=ps;key.topo=2;key.wmask=15;key.has_rt=1;key.samples=1;
        VkPipeline pipeline=Pipeline(key); if (!pipeline) return false;
        Barrier(*input,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Barrier(output,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color.imageView=output.view; color.imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;color.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
        ri.renderArea={{0,0},{width,height}};ri.layerCount=1;ri.colorAttachmentCount=1;ri.pColorAttachments=&color;
        d_.vkCmdBeginRendering(cmd_,&ri);
        VkViewport viewport{0,0,float(width),float(height),0,1};VkRect2D scissor{{0,0},{width,height}};
        d_.vkCmdSetViewport(cmd_,0,1,&viewport);d_.vkCmdSetScissor(cmd_,0,1,&scissor);
        d_.vkCmdBindPipeline(cmd_,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        VkDescriptorImageInfo image{VK_NULL_HANDLE,input->view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo sampler{glow_soft_sampler_,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED};
        VkWriteDescriptorSet writes[2]{};
        for (auto& w:writes) {w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.descriptorCount=1;}
        writes[0].dstBinding=2;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[0].pImageInfo=&image;
        writes[1].dstBinding=6;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;writes[1].pImageInfo=&sampler;
        PushSet(2, writes);
        d_.vkCmdDraw(cmd_,3,1,0,0);d_.vkCmdEndRendering(cmd_);
        Barrier(output,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);input=&output;
    }
    return true;
}

// Optional quality filtering of the traced glow chain. Private working
// images keep the game's surface sizes and other effects unchanged.
void VulkanBackend::InvalidateContinuousGlow(uint32_t data) {
    for (auto& r:continuous_record_) if (r.data==(data&0x7FFFFFFFu)) r.frame=~0u;
}

int VulkanBackend::ContinuousGlowStage(const State& s,const Assembled& a,uint32_t count) {
    if (!s.ps || s.rs[kRsZEnable] || s.rs[kRsStencilEnable] || SimpleState(s,kMBlendEnable) ||
        SimpleState(s,kMColorWriteEnable)!=0x01010101 || SimpleState(s,kMAlphaTestEnable)) return -1;
    uint32_t w,h;SurfaceSize(s.rt,w,h);
    TextureDesc src{},dst{};
    if (!GlowQuad(a,count,w,h) || !Describe(s.rt.header,dst) || dst.format!=6 ||
        !Describe(s.textures[0],src)) return -1;
    auto source=targets_.find(uint64_t(src.data&0x7FFFFFFFu)<<1);
    if (source==targets_.end() || source->second.img.depth || !source->second.drawn) return -1;
    uint64_t fingerprint=1469598103934665603ull;
    const uint8_t* bytes=reinterpret_cast<const uint8_t*>(s.ps->def);
    for (size_t i=0;i<sizeof(s.ps->def);++i) fingerprint=(fingerprint^bytes[i])*1099511628211ull;
    constexpr uint64_t extraction=0x8445E4419501A929ull, blur=0x25965AEEF1CD6DDEull;
    int stage=-1;
    if (w==256 && h==256 && src.width>=640 && src.height>=448 && fingerprint==extraction &&
        s.rs[kRsPsTextureModes]==1) stage=0;
    if (w==256 && h==128 && src.width==256 && src.height==256 && fingerprint==blur) stage=1;
    if (w==128 && h==128 && src.width==256 && src.height==128 && fingerprint==blur) stage=2;
    if (stage<0) return -1;
    if (stage) {
        const auto& previous=continuous_record_[stage-1];
        if (previous.frame!=presents_ || previous.data!=(src.data&0x7FFFFFFFu) ||
            s.rs[kRsPsTextureModes]!=0x8421) return -1;
    }
    // Match the coordinates, axis and offsets too. Similar-sized water,
    // shadow and unrelated screen quads must retain their original shaders.
    for (int tap=0;tap<(stage?4:1);++tap) {
        TextureDesc other{};
        if (!Describe(s.textures[tap],other) || other.data!=src.data ||
            s.tss[tap][kTssAddressU]!=3 || s.tss[tap][kTssAddressV]!=3 ||
            s.tss[tap][kTssMinFilter]!=2 || s.tss[tap][kTssMagFilter]!=2) return -1;
        uint32_t method=(tap&1?kMPsConstant1:kMPsConstant0)+(tap/2)*4;
        // The game alternates equal taps with stronger inner/weaker outer
        // taps. Both have total weight 256/255. Recognize both so a stable
        // continuous filter is used on every frame, rather than alternating
        // with the original sparse blur.
        uint32_t inner=SimpleState(s,kMPsConstant0);
        if (stage && inner!=0x40404040 && inner!=0x55555555) return -1;
        uint32_t expected=inner==0x55555555 && (tap&1)?0x2B2B2B2B:inner;
        if (stage && SimpleState(s,method)!=expected) return -1;
        const auto& e=a.elements[9+tap];
        if (!((a.regs>>(9+tap))&1) || e.format!=16 || e.buffer>=a.buffer_count) return -1;
        const auto& b=a.buffers[e.buffer];
        if (!b.src || !b.stride || size_t(e.offset)+size_t(b.stride)*3+8>b.bytes) return -1;
        const float offsets[]={2.0f/256,6.0f/256,-2.0f/256,-6.0f/256};
        for (int v=0;v<4;++v) {
            float xy[2],uv[2];const auto& pe=a.elements[0];const auto& pb=a.buffers[pe.buffer];
            std::memcpy(xy,pb.src+size_t(v)*pb.stride+pe.offset,8);
            std::memcpy(uv,b.src+size_t(v)*b.stride+e.offset,8);
            float u=xy[0]>float(w)/2?1.0f:0.0f,vv=xy[1]>float(h)/2?1.0f:0.0f;
            if (!stage) {u*=src.width;vv*=src.height;}
            else if (stage==1) vv+=offsets[tap];else u+=offsets[tap];
            float epsilon=stage?0.00001f:0.001f; // Rectangular coordinates are in native pixels.
            if (!std::isfinite(uv[0]) || !std::isfinite(uv[1]) || std::abs(uv[0]-u)>epsilon ||
                std::abs(uv[1]-vv)>epsilon) return -1;
        }
    }
    return stage;
}

bool VulkanBackend::ContinuousGlowPass(Image& input,Image& output,uint32_t w,uint32_t h,int stage,const float* threshold) {
    EndRendering();
    if (output.image && (output.w!=w || output.h!=h)) Retire(output);
    if (!output.image && !CreateImage(output,kColorFormat,w,h,1,1,false,false,
        VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) return false;
    if (!glow_soft_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ci.magFilter=ci.minFilter=VK_FILTER_LINEAR;
        ci.addressModeU=ci.addressModeV=ci.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (d_.vkCreateSampler(dev_,&ci,nullptr,&glow_soft_sampler_)!=VK_SUCCESS) return false;
    }
    constexpr uint64_t vs=0x434F4E54474C5653ull;
    if (!Module(vs,[] {return std::string(
        "float4 main(uint id:SV_VertexID):SV_Position { float2 p=float2((id<<1)&2,id&2); return float4(p*2-1,0,1); }\n");},true)) return false;
    auto footprint=GlowFootprint();
    uint32_t options[]={input.w,input.h,w,h,uint32_t(stage),5u,uint32_t(ngb_pc_setting(7))};
    uint64_t ps=Hash(options,sizeof(options),0x434F4E54474C5053ull);
    if (!Module(ps,[&] {
        std::ostringstream code;code<<std::setprecision(12);
        // the extraction's threshold: the draw's constant, a uniform, so a new value needs no new shader
        code<<"cbuffer GT : register(b0) { float4 threshold; };\n"
            <<"Texture2D tex0 : register(t0);\n"
            <<"float4 main(float4 p:SV_Position):SV_Target {\n"
            <<"float2 ratio=float2("<<double(input.w)/w<<","<<double(input.h)/h<<");\n"
            <<"float2 center=p.xy*ratio; float2 size=ratio;\n";
        // A continuous 16-native-texel box fills the gaps between the four
        // taps. Its variance (21.33) is close to their variance (20).
        if (stage==1) code<<"size.y="<<double(input.h)*footprint.y/256<<";\n";
        if (stage==2) code<<"size.x="<<double(input.w)*footprint.x/256<<";\n";
        code<<"float2 lo=center-size*0.5,hi=center+size*0.5; float4 sum=0;\n"
            <<"[loop] for(int y=int(floor(lo.y));y<int(ceil(hi.y));++y) {\n"
            <<"float wy=max(0.0,min(hi.y,float(y+1))-max(lo.y,float(y)));\n"
            <<"[loop] for(int x=int(floor(lo.x));x<int(ceil(hi.x));++x) {\n"
            <<"float wx=max(0.0,min(hi.x,float(x+1))-max(lo.x,float(x)));\n"
            <<"float4 value=tex0.Load(int3(clamp(int2(x,y),int2(0,0),int2("<<input.w-1<<","<<input.h-1<<")),0));\n";
        if (!stage) code<<"value=saturate(2*(value+threshold)-1);\n";
        code<<"sum+=value*wx*wy; }}\nreturn sum/(size.x*size.y)"<<((stage==1 || stage==2)?"*(256.0/255.0)":"")<<"; }\n";
        return code.str();
    },false)) return false;
    PipeKey key{};key.vs=vs;key.ps=ps;key.topo=2;key.wmask=15;key.has_rt=1;key.samples=1;
    VkPipeline pipeline=Pipeline(key);if (!pipeline) return false;
    VkDeviceSize at=Upload(threshold,16,ringAlign_);   // may submit: before the barriers and the rendering
    Barrier(input,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);Barrier(output,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color.imageView=output.view;color.imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;color.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};ri.renderArea={{0,0},{w,h}};
    ri.layerCount=1;ri.colorAttachmentCount=1;ri.pColorAttachments=&color;d_.vkCmdBeginRendering(cmd_,&ri);
    VkViewport viewport{0,0,float(w),float(h),0,1};VkRect2D scissor{{0,0},{w,h}};
    d_.vkCmdSetViewport(cmd_,0,1,&viewport);d_.vkCmdSetScissor(cmd_,0,1,&scissor);
    d_.vkCmdBindPipeline(cmd_,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
    VkDescriptorImageInfo image{VK_NULL_HANDLE,input.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorBufferInfo constants{ring_,at,16};
    VkWriteDescriptorSet writes[2]{};
    for (auto& w:writes) {w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.descriptorCount=1;}
    writes[0].dstBinding=0;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[0].pBufferInfo=&constants;
    writes[1].dstBinding=2;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[1].pImageInfo=&image;
    PushSet(2, writes);
    d_.vkCmdDraw(cmd_,3,1,0,0);d_.vkCmdEndRendering(cmd_);Barrier(output,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return true;
}

void VulkanBackend::ApplyContinuousGlow(const State& s,const Assembled& a,uint32_t count,Target& rt,const DrawConstants& c) {
    int stage=ContinuousGlowStage(s,a,count);if (stage<0) return;
    TextureDesc desc{};Describe(s.textures[0],desc);
    auto source=targets_.find(uint64_t(desc.data&0x7FFFFFFFu)<<1);
    Image& input=stage?continuous_glow_[stage-1]:source->second.img;
    // Two samples per native axis retain small highlights without running a
    // 160-tap blur on every pixel of a 10x intermediate.
    uint32_t factor=scale_>=2?2:1,w=(stage==2?128:256)*factor,h=(stage?128:256)*factor;
    // At the narrowest width the final 2:1 horizontal reduction was wider
    // than the requested blur. Keep that axis on the extraction grid so
    // its sampling footprint does not overwhelm the smaller kernel.
    if (stage==2) w*=2;
    // A sub-native-texel kernel needs finer input samples too. Refine only
    // the horizontal working grid; all vertical filtering keeps its size.
    w*=2;
    if (!ContinuousGlowPass(input,continuous_glow_[stage],w,h,stage,c.pc[0]))
        ngb_renderer_fatal("Could not filter the continuous glow chain.");
    Image& filtered=continuous_glow_[stage];
    Barrier(filtered,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);Barrier(rt.img,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit blit{};blit.srcSubresource=blit.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
    blit.srcOffsets[1]={int32_t(w),int32_t(h),1};blit.dstOffsets[1]={int32_t(rt.img.w),int32_t(rt.img.h),1};
    d_.vkCmdBlitImage(cmd_,filtered.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        rt.img.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_LINEAR);
    Barrier(filtered,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);Barrier(rt.img,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    rt.drawn=true;rt.ms_stale=rt.samples>1;continuous_record_[stage]={s.rt.data&0x7FFFFFFFu,presents_};
    if (++continuous_hits_[stage]<=4 || continuous_hits_[stage]%600==0) {
        std::fprintf(stderr,"[GLOW-CONTINUOUS] frame %u stage %d source %ux%u working %ux%u target %ux%u %s\n",
            presents_,stage,input.w,input.h,w,h,rt.img.w,rt.img.h,stage?"continuous box":"area-filtered bright extraction");
        auto footprint=GlowFootprint();
        std::fprintf(stderr,"[GLOW-FOOTPRINT] frame %u stage %d aspect %.9g native span %.9g %.9g\n",
            presents_,stage,footprint.aspect,footprint.x,footprint.y);
    }
}
