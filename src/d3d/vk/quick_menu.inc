// Only the carousel crosses from CPU to GPU. The game frame stays on the GPU,
// with the usual frame fences and presentation cadence in both pause modes.
// The background while the quick menu pauses the game: the frame made small (each small pixel the average
// of the area it covers, so nothing shimmers), a Gaussian blur across and down at that size, and the
// result over the frame, a little darker. Only this frame's display copy: no readback, no held picture.
void VulkanBackend::QuickMenuBackground(Image& dst) {
    // The display copy cannot be sampled: blitted first (linear) to at most 1920 wide, which the
    // averaging below reads.
    uint32_t fw = dst.w, fh = dst.h;
    if (fw > 1920) { fh = std::max(1u, uint32_t(uint64_t(fh) * 1920 / fw)); fw = 1920; }
    if (quick_full_.image && (quick_full_.w != fw || quick_full_.h != fh)) Retire(quick_full_);
    if (!quick_full_.image &&
        !CreateImage(quick_full_, kColorFormat, fw, fh, 1, 1, false, false,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) return;
    for (Image* im : {&quick_background_, &quick_blur_})
        if (!im->image &&
            !CreateImage(*im, kColorFormat, 640, 360, 1, 1, false, false,
                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) return;
    const uint64_t vs = 0xF113000000000001ull, down = 0xF113000000000004ull,
                   across = 0xF113000000000005ull, along = 0xF113000000000006ull, over = 0xF113000000000003ull;
    static const char* head = "Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                              "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n";
    Module(down, [] {   // 4 x 4 linear taps over the source area of one small pixel
        return std::string(head) +
            "float4 main(VO i) : SV_Target { float w, h; tex0.GetDimensions(w, h);\n"
            "float2 cell = float2(1.0 / 640.0, 1.0 / 360.0), c0 = i.uv - cell * 0.375; float3 c = 0;\n"
            "for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++)\n"
            "c += tex0.SampleLevel(smp0, c0 + cell * float2(x, y) * 0.25, 0).rgb;\n"
            "return float4(c / 16, 1); }\n";
    }, false);
    auto blur = [&](uint64_t key, const char* dir) {
        Module(key, [&] {
            return std::string(head) + "float4 main(VO i) : SV_Target { float2 d = " + dir + ";\n"
                "float3 c = tex0.Sample(smp0, i.uv).rgb * 0.099908;\n"
    "c += (tex0.Sample(smp0, i.uv + d * 1.476580).rgb + tex0.Sample(smp0, i.uv - d * 1.476580).rgb) * 0.185003;\n"
    "c += (tex0.Sample(smp0, i.uv + d * 3.445530).rgb + tex0.Sample(smp0, i.uv - d * 3.445530).rgb) * 0.136012;\n"
    "c += (tex0.Sample(smp0, i.uv + d * 5.414899).rgb + tex0.Sample(smp0, i.uv - d * 5.414899).rgb) * 0.078177;\n"
    "c += (tex0.Sample(smp0, i.uv + d * 7.384912).rgb + tex0.Sample(smp0, i.uv - d * 7.384912).rgb) * 0.035128;\n"
    "c += (tex0.Sample(smp0, i.uv + d * 9.355775).rgb + tex0.Sample(smp0, i.uv - d * 9.355775).rgb) * 0.012338;\n"
    "c += (tex0.Sample(smp0, i.uv + d * 11.327668).rgb + tex0.Sample(smp0, i.uv - d * 11.327668).rgb) * 0.003387;\n"
                "return float4(c, 1); }\n";
        }, false);
    };
    blur(across, "float2(1.0 / 640.0, 0)");
    blur(along, "float2(0, 1.0 / 360.0)");
    Module(over, [] {
        return std::string(head) + "float4 main(VO i) : SV_Target { return float4(tex0.Sample(smp0, i.uv).rgb * 0.80, 1); }\n";
    }, false);
    auto pass = [&](uint64_t ps, Image& in, Image& out) {
        PipeKey k{};
        k.vs = vs; k.ps = ps; k.topo = 2; k.wmask = 0xF; k.has_rt = 1; k.samples = 1;
        VkPipeline pipe = Pipeline(k);
        if (!pipe) return false;
        Barrier(in, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Barrier(out, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        ca.imageView = out.view; ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ca.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
        ri.renderArea = {{0, 0}, {out.w, out.h}}; ri.layerCount = 1;
        ri.colorAttachmentCount = 1; ri.pColorAttachments = &ca;
        d_.vkCmdBeginRendering(cmd_, &ri);
        VkViewport vp{0, 0, float(out.w), float(out.h), 0, 1};
        VkRect2D sc{{0, 0}, {out.w, out.h}};
        d_.vkCmdSetViewport(cmd_, 0, 1, &vp); d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
        d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
        VkDescriptorImageInfo im{VK_NULL_HANDLE, in.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo sm{quick_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
        VkWriteDescriptorSet w[2]{};
        for (auto& x : w) { x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; x.descriptorCount = 1; }
        w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im;
        w[1].dstBinding = 6; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[1].pImageInfo = &sm;
        PushSet(2, w);
        d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
        d_.vkCmdEndRendering(cmd_);
        return true;
    };
    Barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    Barrier(quick_full_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit b{};
    b.srcSubresource = b.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    b.srcOffsets[1] = {int32_t(dst.w), int32_t(dst.h), 1};
    b.dstOffsets[1] = {int32_t(fw), int32_t(fh), 1};
    d_.vkCmdBlitImage(cmd_, dst.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      quick_full_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &b, VK_FILTER_LINEAR);
    if (pass(down, quick_full_, quick_background_) && pass(across, quick_background_, quick_blur_) &&
        pass(along, quick_blur_, quick_background_))
        pass(over, quick_background_, dst);
    Barrier(dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void VulkanBackend::QuickMenu(Image& dst) {
    int percent = qk_.percent;   // (the frame's, CaptureQuick)
    if (!percent || qk_.layer.size() != size_t(NGB_QUICK_WIDTH) * NGB_QUICK_HEIGHT) return;
    const uint32_t* pixels = qk_.layer.data();
    if (!quick_layer_.image &&
        !CreateImage(quick_layer_, kColorFormat, NGB_QUICK_WIDTH, NGB_QUICK_HEIGHT, 1, 1, false, false,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) return;
    VkDeviceSize at = Upload(pixels, qk_.layer.size() * sizeof(uint32_t), 16);
    Barrier(quick_layer_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy c{};
    c.bufferOffset = at;
    c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    c.imageExtent = {NGB_QUICK_WIDTH, NGB_QUICK_HEIGHT, 1};
    d_.vkCmdCopyBufferToImage(cmd_, ring_, quick_layer_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
    const uint64_t vs = 0xF113000000000001ull, ps = 0xF113000000000002ull;
    Module(vs, [] {
        return std::string("struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "VO main(uint id : SV_VertexID) { VO o; float2 p = float2((id << 1) & 2, id & 2);\n"
                           "o.uv = p; o.pos = float4(p * 2 - 1, 0, 1); return o; }\n");
    }, true);
    Module(ps, [] {
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                           "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "float4 main(VO i) : SV_Target { return tex0.Sample(smp0, i.uv); }\n");
    }, false);
    PipeKey k{};
    k.vs = vs; k.ps = ps; k.topo = 2; k.wmask = 0xF; k.has_rt = 1; k.samples = 1;
    k.blend = 1; k.src = 1; k.dst = 0x303;
    VkPipeline pipe = Pipeline(k);
    if (!quick_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter = ci.minFilter = VK_FILTER_LINEAR;
        ci.addressModeU = ci.addressModeV = ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        d_.vkCreateSampler(dev_, &ci, nullptr, &quick_sampler_);
    }
    if (!pipe || !quick_sampler_) return;
    // Apply the background effect before the crisp menu. Waiting for an
    // action and Quick Menu Pause Off both bypass this pass entirely.
    if (qk_.frozen) QuickMenuBackground(dst);
    Barrier(quick_layer_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = dst.view; ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {dst.w, dst.h}}; ri.layerCount = 1;
    ri.colorAttachmentCount = 1; ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    /* The Xbox front buffer has non-square display pixels. Match the final
     * display aspect, so bottles, weapons and arrows retain their proportions. */
    int mode = ngb_pc_setting(7);
    float aspect = mode == 0 ? 4.0f / 3 : mode == 2 ? 16.0f / 10 : mode == 3 ? 64.0f / 27 : mode == 4 ? 32.0f / 9
                 : 16.0f / 9;
    float sx = float(dst.w) / (720 * aspect), sy = float(dst.h) / 720;
    float scale = percent / 100.0f;
    sx *= scale; sy *= scale;
    VkViewport vp{0, float(dst.h) - NGB_QUICK_HEIGHT * sy,
                  NGB_QUICK_WIDTH * sx, NGB_QUICK_HEIGHT * sy, 0, 1};
    VkRect2D sc{{0, 0}, {dst.w, dst.h}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vp); d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im{VK_NULL_HANDLE, quick_layer_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{quick_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[2]{};
    for (auto& x : w) { x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; x.descriptorCount = 1; }
    w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im;
    w[1].dstBinding = 6; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[1].pImageInfo = &sm;
    PushSet(2, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
    Barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

// The frame counter (F10) in the game's letters (src/fps_text.c), blended into the top left corner of the picture
// shown: a premultiplied picture uploaded when it changes, drawn as the quick menu's layer is.
void VulkanBackend::FpsCounter(Image& dst) {
    if (!qk_.fps) return;
    ngb_fps_target_h = int(dst.h);   // (lettered at this height from the next change)
    if (!fps_layer_.image) {
        if (!CreateImage(fps_layer_, kColorFormat, kFpsWidth, kFpsHeight, 1, 1, false, false,
                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) return;
        if (fps_pending_.empty()) fps_pending_.assign(size_t(kFpsWidth) * kFpsHeight, 0);   // clear until lettered
    }
    if (!fps_pending_.empty()) {
        VkDeviceSize at = Upload(fps_pending_.data(), fps_pending_.size() * sizeof(uint32_t), 16);
        Barrier(fps_layer_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy c{};
        c.bufferOffset = at;
        c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        c.imageExtent = {uint32_t(kFpsWidth), uint32_t(kFpsHeight), 1};
        d_.vkCmdCopyBufferToImage(cmd_, ring_, fps_layer_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
        fps_pending_.clear();
    }
    if (!fps_lettered_) return;   // the presenter's own letters meanwhile (the game's font not loaded yet)
    const uint64_t vs = 0xF113000000000001ull, ps = 0xF113000000000002ull;   // the quick menu's shaders
    PipeKey k{};
    k.vs = vs; k.ps = ps; k.topo = 2; k.wmask = 0xF; k.has_rt = 1; k.samples = 1;
    k.blend = 1; k.src = 1; k.dst = 0x303;
    Module(vs, [] {
        return std::string("struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "VO main(uint id : SV_VertexID) { VO o; float2 p = float2((id << 1) & 2, id & 2);\n"
                           "o.uv = p; o.pos = float4(p * 2 - 1, 0, 1); return o; }\n");
    }, true);
    Module(ps, [] {
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                           "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "float4 main(VO i) : SV_Target { return tex0.Sample(smp0, i.uv); }\n");
    }, false);
    VkPipeline pipe = Pipeline(k);
    if (!quick_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter = ci.minFilter = VK_FILTER_LINEAR;
        ci.addressModeU = ci.addressModeV = ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        d_.vkCreateSampler(dev_, &ci, nullptr, &quick_sampler_);
    }
    if (!pipe || !quick_sampler_) return;
    Barrier(fps_layer_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = dst.view; ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {dst.w, dst.h}}; ri.layerCount = 1;
    ri.colorAttachmentCount = 1; ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    // lettered with square pixels at the picture's height; the picture is stretched to the display's aspect
    int mode = ngb_pc_setting(7);
    float aspect = mode == 0 ? 4.0f / 3 : mode == 2 ? 16.0f / 10 : mode == 3 ? 64.0f / 27 : mode == 4 ? 32.0f / 9
                 : 16.0f / 9;
    float sx = float(dst.w) / (float(dst.h) * aspect), up;
    ngb_fps_cap(int(dst.h), &up);   // (lettered at most 40 pixels tall: drawn up larger)
    VkViewport vp{dst.h * 0.012f * sx, dst.h * 0.004f, kFpsWidth * sx * up, kFpsHeight * up, 0, 1};
    VkRect2D sc{{0, 0}, {dst.w, dst.h}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vp); d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im{VK_NULL_HANDLE, fps_layer_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{quick_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[2]{};
    for (auto& x : w) { x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; x.descriptorCount = 1; }
    w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im;
    w[1].dstBinding = 6; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[1].pImageInfo = &sm;
    PushSet(2, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
    Barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    nv2a_fps_drawn_outside = 1;
}
