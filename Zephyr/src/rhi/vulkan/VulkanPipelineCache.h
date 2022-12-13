#pragma once
#include "VulkanCommon.h"
#include "rhi/Handle.h"
#include "rhi/RHIEnums.h"
#include "rhi/RHIRenderTarget.h"

namespace Zephyr
{
    class VulkanDriver;
    class VulkanShaderSet;
    class VulkanBuffer;
    class RHIBuffer;
    class RHITexture;
    class VulkanRenderTarget;

    struct DescriptorState
    {
        uint32_t index;
        bool     bound;
    };

    struct BindingDescriptor
    {
        DescriptorType type;
        HandleID       handle;
        // for texture views
        ViewRange      range = ViewRange::INVALID_RANGE;
        // for dynamic buffer
        int            offset = -1;

        // note that offsets doens't get included here
        bool operator==(const BindingDescriptor& rhs) const { return rhs.type == type && rhs.handle == handle && range == rhs.range; }
    };

    struct hash_fn_bd
    {
        size_t operator()(const BindingDescriptor& rhs) const
        {
            return std::hash<uint32_t>()((uint32_t)rhs.type) ^ std::hash<uint32_t>()(rhs.handle) ^
                   std::hash<uint32_t>()(rhs.offset);
        }
    };

    struct PipelineLayoutKey
    {
        std::vector<std::vector<BindingDescriptor>> set;
    };

    // if the cache key is the same, we could reuse pipeline no problem
    struct PipelineCacheKey
    {
        RenderTargetDescription rtDescriptor;
        VulkanShaderSet*        shader;
        RasterState             raster;

        bool operator==(const PipelineCacheKey& rhs) const
        {
            return rhs.rtDescriptor == rtDescriptor && rhs.shader == shader && rhs.raster == raster;
        }
    };

    struct hash_fn_pck
    {
        size_t operator()(const PipelineCacheKey& t) const
        {
            return hash_fn_rt()(t.rtDescriptor) ^ std::hash<void*>()(t.shader) ^
                   std::hash<uint32_t>()((uint32_t)t.raster.cull) ^ std::hash<uint32_t>()((uint32_t)t.raster.frontFace);
        }
    };

    struct DescriptorSetCacheKey
    {
        VkDescriptorSetLayout          layout;
        std::vector<BindingDescriptor> Bindings;

        bool operator==(const DescriptorSetCacheKey& rhs) const
        {
            if (rhs.layout != layout)
            {
                return false;
            }
            if (rhs.Bindings.size() != Bindings.size())
            {
                return false;
            }

            for (uint32_t i = 0; i < Bindings.size(); i++)
            {
                if (!(Bindings[i] == rhs.Bindings[i]))
                {
                    return false;
                }
            }
            return true;
        }
    };

    struct hash_fn_dsck
    {
        size_t operator()(const DescriptorSetCacheKey& rhs) const
        {
            size_t r = std::hash<void*>()(rhs.layout);
            for (auto& b : rhs.Bindings)
            {
                r ^= hash_fn_bd()(b);
            }
            return r;
        }
    };

    struct PushConstantDescriptor
    {
        uint32_t    offset;
        uint32_t    size;
        void*       data;
        ShaderStage stage;
    };
    /*
        The VulkanPipelineCache records all the current state of the pipeline, like
        depth testing, raster state, blending, currently bound shader, currently bound renderpass, etc.

        VkPipeline will be generated on runtime based on different combination of configurations. This might
        not be the most optimal as pipeline generation is typically expensive, but this works best with
        the frame graph implementaion.

    */

    // this is used to determin if two pipelines are the same
    struct PipelineKey
    {
        PipelineStage pipeline;
    };

    class VulkanPipelineCache final
    {
    public:
        VulkanPipelineCache(VulkanDriver* driver);
        void Shutdown();
        ~VulkanPipelineCache();

        void BindShaderSet(VulkanShaderSet* shader);
        void BindRenderPass(VulkanRenderTarget* rt);
        void BindStorageBufferDynamic(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding, int offset);
        void BindStorageBuffer(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding);
        void BindSampler2D(Handle<RHITexture> texture, const ViewRange& range, uint32_t set, uint32_t binding);
        void BindSampler2DArray(Handle<RHITexture> texture, uint32_t set, uint32_t binding);
        void BindSamplerCubemap(Handle<RHITexture> texture, uint32_t set, uint32_t binding);
        void BindStorageImage(Handle<RHITexture> texture, const ViewRange& range, uint32_t set, uint32_t binding);
        void BindPushConstant(VkCommandBuffer cb, uint32_t offset, uint32_t size, ShaderStage stage, void* data);
        void SetRaster(const RasterState& raster);
        void SetViewportScissor(VkCommandBuffer cb, const Viewport& viewport, const Scissor& scissor);

        void Begin(VkCommandBuffer cb);
        void BindDescriptor(VkCommandBuffer cb);
        // void End(VkCommandBuffer cb);
        void Reset();

    private:
        VkPipeline CreatePipelineGraphics();
        VkPipeline CreatePipelineCompute();

    private:
        VulkanDriver* m_Driver;

        VulkanShaderSet*    m_BoundShader     = nullptr;
        VulkanRenderTarget* m_BoundRenderPass = nullptr;

        PipelineLayoutKey                   m_CurrentBinding;
        std::vector<PushConstantDescriptor> m_BoundPushConstantList {};
        Viewport                            m_Viewport {};
        Scissor                             m_Scissor {};

        RasterState m_CurrentRasterState {Culling::None, FrontFace::CounterClockwise};
        // std::unordered_map<PipelineKey, VkPipeline> m_PipelineCache;
        std::unordered_map<PipelineCacheKey, VkPipeline, hash_fn_pck> m_PipelineCacheGraphics;
        std::unordered_map<VulkanShaderSet*, VkPipeline>              m_PipelineCacheCompute;
        std::unordered_map<VkPipeline,
                           std::vector<std::unordered_map<DescriptorSetCacheKey, DescriptorState, hash_fn_dsck>>>
            m_PipelineDescriptorCache;

        std::vector<VkDescriptorSet> m_DescriptorSetCache;

        VkPipeline m_CurrentBoundPipelineGraphics = VK_NULL_HANDLE;
        VkPipeline m_CurrentBoundPipelineCompute  = VK_NULL_HANDLE;

        bool m_FreshPipelineGraphics = true;
        bool m_FreshPipelineCompute  = true;
    };
} // namespace Zephyr