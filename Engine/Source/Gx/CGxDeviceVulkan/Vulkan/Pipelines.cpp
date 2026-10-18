// The graphics pipelines a draw binds: the module's life and the record of one draw.
#include "Engine/Source/Gx/CGxDeviceVulkan/Vulkan/Pipelines/Internal.hpp"

#include <cstring>
#include <new>

namespace gxvk::vk::pipelines::detail
{
    State s;
}

using namespace gxvk::vk::pipelines::detail;

namespace
{
    /// The set-0 images of a draw for a layout: the draw's texture at each slot of the mask, or the
    /// white image of the slot's kind when the draw has none there. False when a texture is bound
    /// of the other kind (a 2D image under a cube sampler or the reverse), which the module cannot
    /// sample.
    bool StageImages(const Draw& draw, const LayoutEntry& layout, uint32_t cubeMask, Texture* out)
    {
        for (uint32_t slot = 0; slot < kTextureStages; ++slot)
        {
            if (!(layout.mask & (1u << slot))) continue;
            const bool wantCube = (cubeMask & (1u << slot)) != 0;
            if ((draw.textureMask & (1u << slot)) && draw.textures[slot].view != VK_NULL_HANDLE)
            {
                if (draw.textures[slot].cube != wantCube) return false;
                out[slot] = draw.textures[slot];
                continue;
            }
            const texture::Image* white = White(wantCube);
            if (!white) return false;
            out[slot].view       = white->view;
            out[slot].imageId    = white->id;
            out[slot].samplerKey = kWhiteSamplerKey;
            out[slot].cube       = wantCube;
        }
        return true;
    }
}

namespace gxvk::vk::pipelines
{
    bool Init()
    {
        if (s.ready) return true;
        const context::Handles* h = context::Get();
        if (!h || !h->dapi || !h->iapi)
        {
            Write(Warn, "vulkan: pipelines: Init() without a device; declining");
            return false;
        }
        s.device = h->device;
        s.dapi   = h->dapi;
        const loader::Device& d = *s.dapi;

        VkPhysicalDeviceProperties props = {};
        h->iapi->GetPhysicalDeviceProperties(h->physical, &props);
        s.anisoMax  = h->samplerAnisotropy ? props.limits.maxSamplerAnisotropy : 1.0f;
        s.regsAlign = props.limits.minUniformBufferOffsetAlignment ? static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment) : 256u;
        s.wireframe = h->fillModeNonSolid;
        if (props.limits.maxUniformBufferRange < kSliceBytesLarge)
        {
            Write(Error, "vulkan: pipelines: maxUniformBufferRange %u cannot hold a %u-byte slice; declining", props.limits.maxUniformBufferRange, kSliceBytesLarge);
            return false;
        }

        s.entries = new (std::nothrow) Entry[kMaxPipelines];
        if (!s.entries)
        {
            Write(Error, "vulkan: pipelines: cannot allocate the pipeline table");
            return false;
        }

        if (!CreatePrograms() || !CreateRegsLayout() || !CreateDescriptorObjects())
        {
            Shutdown();
            return false;
        }

        // The set-1 sets, one per uniform chunk, for the session.
        VkDescriptorPoolSize size = {};
        size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        size.descriptorCount = frame::kFramesInFlight * kMaxRegsChunks;
        VkDescriptorPoolCreateInfo pool = {};
        pool.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool.maxSets       = frame::kFramesInFlight * kMaxRegsChunks;
        pool.poolSizeCount = 1;
        pool.pPoolSizes    = &size;
        VkResult r = d.CreateDescriptorPool(s.device, &pool, nullptr, &s.regsPool);
        if (r != VK_SUCCESS)
        {
            s.regsPool = VK_NULL_HANDLE;
            Write(Error, "vulkan: pipelines: vkCreateDescriptorPool (set 1) failed: %s", ResultName(r));
            Shutdown();
            return false;
        }

        VkPipelineCacheCreateInfo cache = {};
        cache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        if (d.CreatePipelineCache(s.device, &cache, nullptr, &s.cache) != VK_SUCCESS) s.cache = VK_NULL_HANDLE;

        if (!memory::CreateBuffer(kConstantBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memory::Where::Stream, s.constant) || !s.constant.block.mapped)
        {
            Write(Error, "vulkan: pipelines: no buffer for the constant stream");
            Shutdown();
            return false;
        }
        std::memset(s.constant.block.mapped, 0, kConstantBytes);
        std::memset(static_cast<uint8_t*>(s.constant.block.mapped) + kConstantColourOffset, 0xFF, 16);
        memory::FlushHost(s.constant.block);

        s.ready = true;
        Write(Debug, "vulkan: pipelines: ready (%u own vertex and %u own fragment programs, %u-byte push constants, %u set-0 sets per frame, uniform slices of %u / %u bytes at alignment %u in %u KiB chunks)",
              kVertexPrograms - 1, kFragmentPrograms - 1, kPushBytes, kSetsPerSlot, kSliceBytesSmall, kSliceBytesLarge, s.regsAlign, kRegsChunkBytes / 1024);
        return true;
    }

    void Shutdown()
    {
        if (!s.dapi || !s.device) return;
        const loader::Device& d = *s.dapi;
        DropPipelines();
        DestroySamplers();
        DestroyDescriptorObjects();
        if (s.regsPool != VK_NULL_HANDLE) d.DestroyDescriptorPool(s.device, s.regsPool, nullptr);
        DestroyLayouts();
        memory::DestroyBuffer(s.constant);
        if (s.cache != VK_NULL_HANDLE) d.DestroyPipelineCache(s.device, s.cache, nullptr);
        DestroyPrograms();
        delete[] s.entries;
        s = State();
    }

    bool Record(VkCommandBuffer cmd, VkRenderPass pass, uint32_t passGeneration, uint64_t serial, uint32_t slot, const Draw& draw)
    {
        if (!s.ready || !cmd || pass == VK_NULL_HANDLE)
        {
            ++s.stats.skipped;
            return false;
        }
        if (draw.streamCount == 0 || draw.streamCount > kMaxStreams || !(draw.attribMask & (1u << kAttribPosition)) || draw.count == 0)
        {
            ++s.stats.skipped;
            return false;
        }

        // A new render pass generation: the pipelines were built against a pass that no longer
        // exists. The frame made the device idle before it dropped that pass.
        if (passGeneration != s.passGen)
        {
            DropPipelines();
            s.passGen = passGeneration;
        }

        // The sampler-slot set: the pixel record's for a translated module, slots 0 and 1 for the
        // fixed-function stages, slot 0 alone otherwise.
        const bool translated = TranslatedFamily(draw.vertex) || TranslatedFamily(draw.fragment);
        // The fixed-function fragment program reads set 1 too, for its fog (Shaders/Ff.frag).
        const bool regs = translated || draw.fragment == FragmentProgram::FixedFunction;
        uint32_t samplerMask = 1u;
        uint32_t cubeMask    = 0;
        if (draw.fragment == FragmentProgram::Translated && draw.fragmentRecord)
        {
            samplerMask = draw.fragmentRecord->samplerMask;
            cubeMask    = draw.fragmentRecord->cubeMask;
        }
        else if (draw.fragment == FragmentProgram::FixedFunction || draw.fragment == FragmentProgram::FixedFunctionTranslated)
            samplerMask = 3u;
        uint32_t layoutIndex = 0;
        if (!LayoutFor(samplerMask, layoutIndex))
        {
            ++s.stats.skipped;
            return false;
        }

        Key k;
        std::memset(&k, 0, sizeof k);
        k.pass        = pass;
        k.passGen     = passGeneration;
        k.streamCount = draw.streamCount;
        for (uint32_t i = 0; i < draw.streamCount; ++i) k.stride[i] = draw.streams[i].stride;
        k.attribMask = draw.attribMask;
        for (uint32_t a = 0; a < kAttribCount; ++a)
            if (draw.attribMask & (1u << a)) k.attribs[a] = draw.attribs[a];
        k.topology       = draw.topology;
        k.blend          = draw.blend;
        k.depthTest      = draw.depthTest ? 1u : 0u;
        k.depthWrite     = draw.depthWrite ? 1u : 0u;
        k.depthFunc      = draw.depthFunc;
        k.cull           = draw.cull;
        k.colourMask     = draw.colourMask & 0xFu;
        k.wireframe      = (draw.wireframe && s.wireframe) ? 1u : 0u;
        k.vertex         = static_cast<uint32_t>(draw.vertex);
        k.fragment       = static_cast<uint32_t>(draw.fragment);
        k.vertexRecord   = draw.vertex == VertexProgram::Translated ? manifest::IndexOf(draw.vertexRecord) : manifest::kNoRecord;
        k.fragmentRecord = draw.fragment == FragmentProgram::Translated ? manifest::IndexOf(draw.fragmentRecord) : manifest::kNoRecord;
        k.layoutIndex    = layoutIndex;

        const Entry* entry = Lookup(k);
        if (!entry || entry->pipeline == VK_NULL_HANDLE)
        {
            ++s.stats.skipped;
            return false;
        }
        const LayoutEntry& layout = s.layouts[layoutIndex];

        // Set 0: the images of the mask's slots (the fragment programs sample what they declare
        // unconditionally, so every slot binds a valid image of its kind).
        Texture stages[kTextureStages];
        if (!StageImages(draw, layout, cubeMask, stages))
        {
            ++s.stats.cubeMismatch;
            ++s.stats.skipped;
            return false;
        }
        const VkDescriptorSet set0 = DescriptorFor(slot, serial, layout, stages);
        if (set0 == VK_NULL_HANDLE)
        {
            ++s.stats.skipped;
            return false;
        }

        // Set 1: the constant slice, for the translated family and the fixed-function fragment program.
        VkDescriptorSet set1    = VK_NULL_HANDLE;
        uint32_t        offset1 = 0;
        if (regs && !RegsSlice(slot, serial, draw, set1, offset1))
        {
            ++s.stats.skipped;
            return false;
        }

        const loader::Device& d = *s.dapi;
        Bound& b = s.bound;
        if (b.cmd != cmd || b.serial != serial)
        {
            b        = Bound();
            b.cmd    = cmd;
            b.serial = serial;
        }

        if (b.pipeline != entry->pipeline)
        {
            d.CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, entry->pipeline);
            b.pipeline = entry->pipeline;
        }

        // A layout change makes every bound set and the push range stale.
        if (b.layout != layout.layout)
        {
            b.layout = layout.layout;
            b.set0   = VK_NULL_HANDLE;
            b.set1   = VK_NULL_HANDLE;
            b.have1  = false;
            b.pushed = false;
        }
        if (b.set0 != set0)
        {
            d.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout.layout, 0, 1, &set0, 0, nullptr);
            b.set0 = set0;
        }
        if (regs && (!b.have1 || b.set1 != set1 || b.offset1 != offset1))
        {
            d.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout.layout, 1, 1, &set1, 1, &offset1);
            b.set1    = set1;
            b.offset1 = offset1;
            b.have1   = true;
        }

        VkBuffer     vbs[kMaxStreams + 1];
        VkDeviceSize offsets[kMaxStreams + 1];
        uint32_t     vbCount = draw.streamCount;
        for (uint32_t i = 0; i < draw.streamCount; ++i)
        {
            vbs[i]     = draw.streams[i].buffer;
            offsets[i] = draw.streams[i].offset;
        }
        // The constant stream rides along at binding streamCount whenever a pipeline of this layout
        // may read it; a pipeline that declares no such binding ignores the extra buffer.
        vbs[vbCount]     = s.constant.handle;
        offsets[vbCount] = 0;
        ++vbCount;
        bool same = b.vbCount == vbCount;
        for (uint32_t i = 0; same && i < vbCount; ++i) same = b.vbs[i] == vbs[i] && b.offsets[i] == offsets[i];
        if (!same)
        {
            d.CmdBindVertexBuffers(cmd, 0, vbCount, vbs, offsets);
            b.vbCount = vbCount;
            for (uint32_t i = 0; i < vbCount; ++i)
            {
                b.vbs[i]     = vbs[i];
                b.offsets[i] = offsets[i];
            }
        }

        if (!b.placed || std::memcmp(&b.viewport, &draw.viewport, sizeof draw.viewport) != 0)
        {
            d.CmdSetViewport(cmd, 0, 1, &draw.viewport);
            b.viewport = draw.viewport;
        }
        if (!b.placed || std::memcmp(&b.scissor, &draw.scissor, sizeof draw.scissor) != 0)
        {
            d.CmdSetScissor(cmd, 0, 1, &draw.scissor);
            b.scissor = draw.scissor;
        }
        b.placed = true;

        if (!b.biasSet || b.depthBias != draw.depthBias)
        {
            d.CmdSetDepthBias(cmd, draw.depthBias, 0.0f, 0.0f);
            b.depthBias = draw.depthBias;
            b.biasSet   = true;
        }
        if (!b.blendSet || std::memcmp(b.blendConstant, draw.blendConstant, sizeof draw.blendConstant) != 0)
        {
            d.CmdSetBlendConstants(cmd, draw.blendConstant);
            std::memcpy(b.blendConstant, draw.blendConstant, sizeof b.blendConstant);
            b.blendSet = true;
        }

        float push[kPushBytes / sizeof(float)];
        std::memcpy(push, draw.xform, sizeof draw.xform);
        std::memcpy(push + 16, draw.params, sizeof draw.params);
        std::memcpy(push + 20, draw.tex, sizeof draw.tex);
        if (!b.pushed || std::memcmp(b.push, push, sizeof push) != 0)
        {
            d.CmdPushConstants(cmd, layout.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, kPushBytes, push);
            std::memcpy(b.push, push, sizeof push);
            b.pushed = true;
        }

        if (draw.indexed)
        {
            if (draw.indexBuffer == VK_NULL_HANDLE)
            {
                ++s.stats.skipped;
                return false;
            }
            if (b.ib != draw.indexBuffer)
            {
                d.CmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
                b.ib = draw.indexBuffer;
            }
            d.CmdDrawIndexed(cmd, draw.count, 1, draw.firstIndex, 0, 0);
        }
        else
        {
            d.CmdDraw(cmd, draw.count, 1, 0, 0);
        }
        ++s.stats.drawn;
        return true;
    }

    void GetStats(Stats& out) { out = s.stats; }
}
