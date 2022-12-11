#include "VulkanPipelineCache.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDriver.h"
#include "VulkanRenderTarget.h"
#include "VulkanShaderSet.h"
#include "VulkanTexture.h"
#include "VulkanUtil.h"
#include "rhi/RHIBuffer.h"

namespace Zephyr
{
    VulkanPipelineCache::VulkanPipelineCache(VulkanDriver* driver) : m_Driver(driver) {}

    void VulkanPipelineCache::Shutdown()
    {
        for (auto& pipeline : m_PipelineCacheGraphics)
        {
            vkDestroyPipeline(m_Driver->GetContext()->Device(), pipeline.second, nullptr);
        }
        for (auto& pipeline : m_PipelineCacheCompute)
        {
            vkDestroyPipeline(m_Driver->GetContext()->Device(), pipeline.second, nullptr);
        }
    }

    VulkanPipelineCache::~VulkanPipelineCache() {}

    void VulkanPipelineCache::BindShaderSet(VulkanShaderSet* shader) { m_BoundShader = shader; }

    void VulkanPipelineCache::BindRenderPass(VulkanRenderTarget* rt) { m_BoundRenderPass = rt; }

    void
    VulkanPipelineCache::BindStorageBufferDynamic(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding, int offset)
    {
        if (m_CurrentBinding.set.size() < set + 1)
        {
            m_CurrentBinding.set.resize(set + 1);
        }
        if (m_CurrentBinding.set[set].size() < binding + 1)
        {
            m_CurrentBinding.set[set].resize(binding + 1);
        }

        m_CurrentBinding.set[set][binding] = {DescriptorType::StorageBufferDynamic, buffer.GetID(), offset};
    }

    void VulkanPipelineCache::BindStorageBuffer(Handle<RHIBuffer> buffer, uint32_t set, uint32_t binding)
    {
        if (m_CurrentBinding.set.size() < set + 1)
        {
            m_CurrentBinding.set.resize(set + 1);
        }
        if (m_CurrentBinding.set[set].size() < binding + 1)
        {
            m_CurrentBinding.set[set].resize(binding + 1);
        }

        m_CurrentBinding.set[set][binding] = {DescriptorType::StorageBuffer, buffer.GetID()};
    }

    void VulkanPipelineCache::BindSampler2D(Handle<RHITexture> texture, uint32_t set, uint32_t binding)
    {

        if (m_CurrentBinding.set.size() < set + 1)
        {
            m_CurrentBinding.set.resize(set + 1);
        }
        if (m_CurrentBinding.set[set].size() < binding + 1)
        {
            m_CurrentBinding.set[set].resize(binding + 1);
        }

        m_CurrentBinding.set[set][binding] = {DescriptorType::CombinedImageSampler, texture.GetID()};
    }

    void VulkanPipelineCache::BindSampler2DArray(Handle<RHITexture> texture, uint32_t set, uint32_t binding)
    {

        if (m_CurrentBinding.set.size() < set + 1)
        {
            m_CurrentBinding.set.resize(set + 1);
        }
        if (m_CurrentBinding.set[set].size() < binding + 1)
        {
            m_CurrentBinding.set[set].resize(binding + 1);
        }

        m_CurrentBinding.set[set][binding] = {DescriptorType::CombinedImageSampler, texture.GetID()};
    }

    void VulkanPipelineCache::BindSamplerCubemap(Handle<RHITexture> texture, uint32_t set, uint32_t binding)
    {

        if (m_CurrentBinding.set.size() < set + 1)
        {
            m_CurrentBinding.set.resize(set + 1);
        }
        if (m_CurrentBinding.set[set].size() < binding + 1)
        {
            m_CurrentBinding.set[set].resize(binding + 1);
        }
        m_CurrentBinding.set[set][binding] = {DescriptorType::CombinedImageSampler, texture.GetID()};
    }

    void VulkanPipelineCache::BindStorageImage(Handle<RHITexture> texture, uint32_t set, uint32_t binding)
    {
        if (m_CurrentBinding.set.size() < set)
        {
            m_CurrentBinding.set.resize(set);
        }
        if (m_CurrentBinding.set[set].size() < binding)
        {
            m_CurrentBinding.set[set].resize(binding);
        }

        m_CurrentBinding.set[set][binding] = {DescriptorType::StorageImage, texture.GetID()};
    }

    void VulkanPipelineCache::BindPushConstant(VkCommandBuffer cb,
                                               uint32_t        offset,
                                               uint32_t        size,
                                               ShaderStage     stage,
                                               void*           data)
    {
        auto& pc  = m_BoundPushConstantList.emplace_back();
        pc.data   = data;
        pc.size   = size;
        pc.offset = offset;
        pc.stage  = stage;
    }

    void VulkanPipelineCache::SetRaster(const RasterState& raster) { m_CurrentRasterState = raster; }

    void VulkanPipelineCache::SetViewportScissor(VkCommandBuffer cb, const Viewport& viewport, const Scissor& scissor)
    {
        m_Viewport = viewport;
        m_Scissor  = scissor;

        VkViewport vp {};
        vp.x        = viewport.left;
        vp.y        = viewport.bottom;
        vp.width    = viewport.width;
        vp.height   = viewport.height;
        vp.minDepth = 0.;
        vp.maxDepth = 1.;

        VkRect2D s {};
        s.extent.width  = scissor.width;
        s.extent.height = scissor.height;
        s.offset.x      = scissor.x;
        s.offset.y      = scissor.y;

        vkCmdSetViewport(cb, 0, 1, &vp);
        vkCmdSetScissor(cb, 0, 1, &s);
    }

    void VulkanPipelineCache::Begin(VkCommandBuffer cb)
    {
        assert(m_BoundShader);
        assert(m_BoundShader->GetPipelineType() == PipelineTypeBits::Compute || m_BoundRenderPass);
        assert(m_Viewport.IsValid());
        assert(m_Scissor.IsValid());
        auto       renderPass = m_BoundRenderPass->GetRenderPass();
        VkPipeline pip        = VK_NULL_HANDLE;
        if (m_BoundShader->GetPipelineType() == PipelineTypeBits::Compute)
        {
            pip = CreatePipelineCompute();
            assert(pip != VK_NULL_HANDLE);
            if (pip == m_CurrentBoundPipelineCompute)
            {
                return;
            }
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pip);
            m_FreshPipelineCompute = true;
        }
        if (m_BoundShader->GetPipelineType() == PipelineTypeBits::Graphics)
        {
            pip = CreatePipelineGraphics();
            assert(pip != VK_NULL_HANDLE);
            if (pip == m_CurrentBoundPipelineGraphics)
            {
                return;
            }
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pip);
            m_FreshPipelineGraphics = true;
        }
        m_CurrentBoundPipelineGraphics = pip;
    }

    // TODO: this is currently not very readable nor very efficient. need refactor
    void VulkanPipelineCache::BindDescriptor(VkCommandBuffer cb)
    {
        for (auto& pc : m_BoundPushConstantList)
        {
            vkCmdPushConstants(cb,
                               m_BoundShader->GetPipelineLayout(),
                               VulkanUtil::GetShaderStageFlags(pc.stage),
                               pc.offset,
                               pc.size,
                               (uint8_t*)pc.data + pc.offset);
        }
        m_BoundPushConstantList.clear();

        m_FreshPipelineGraphics       = false;
        auto pipelineLayoutDescriptor = m_BoundShader->GetPipelineLayoutDescriptor();
        auto layouts                  = m_BoundShader->GetDescriptorLayouts();

        uint32_t setCount = pipelineLayoutDescriptor.layouts.size();

        VkPipeline          pipeline  = m_BoundShader->GetPipelineType() == PipelineTypeBits::Graphics ?
                                            m_CurrentBoundPipelineGraphics :
                                            m_CurrentBoundPipelineCompute;
        VkPipelineBindPoint bindPoint = m_BoundShader->GetPipelineType() == PipelineTypeBits::Graphics ?
                                            VK_PIPELINE_BIND_POINT_GRAPHICS :
                                            VK_PIPELINE_BIND_POINT_COMPUTE;

        for (uint32_t set = 0; set < setCount; set++)
        {
            auto setDescriptor = pipelineLayoutDescriptor.layouts[set];
            auto setLayout     = layouts[set];

            DescriptorSetCacheKey cacheKey {};
            cacheKey.layout = setLayout;

            std::vector<uint32_t> offsets;

            cacheKey.Bindings.resize(setDescriptor.bindings.size());

            for (uint32_t binding = 0; binding < setDescriptor.bindings.size(); binding++)
            {
                auto bindingDescriptor            = setDescriptor.bindings[binding];
                cacheKey.Bindings[binding].handle = m_CurrentBinding.set[set][binding].handle;
                cacheKey.Bindings[binding].type   = m_CurrentBinding.set[set][binding].type;
                auto offset                       = m_CurrentBinding.set[set][binding].offset;

                if (offset != -1)
                {
                    offsets.push_back(offset);
                }
            }

            auto iter = m_PipelineDescriptorCache.find(pipeline);

            if (iter == m_PipelineDescriptorCache.end())
            {
                m_PipelineDescriptorCache.insert({pipeline, {}});
                m_PipelineDescriptorCache[pipeline].resize(setCount);
            }

            // this is primarily for different materials. for instance, a car model would have different
            // albedo texture than a box model, so we need a descriptor set for each material
            auto& setCache = m_PipelineDescriptorCache[pipeline][set];

            auto setIter = setCache.find(cacheKey);

            if (setIter != setCache.end())
            {
                assert(setIter->second.index < m_DescriptorSetCache.size());

                auto& setCacheKey = setIter->first;

                auto descriptor = m_DescriptorSetCache[setIter->second.index];
                if (!setIter->second.bound)
                {
                    vkCmdBindDescriptorSets(cb,
                                            bindPoint,
                                            m_BoundShader->GetPipelineLayout(),
                                            set,
                                            1,
                                            &descriptor,
                                            offsets.size(),
                                            offsets.data());
                }

                // clear other bound
                for (auto& iter : setCache)
                {
                    iter.second.bound = false;
                }
                setIter->second.bound = true;
            }
            else
            {
                VkDescriptorSet descriptorSet;
                // create descriptor set and update
                VkDescriptorSetAllocateInfo allocInfo {};
                allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool     = m_Driver->GetContext()->GetDescriptorPool();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts        = &setLayout;

                VK_CHECK(vkAllocateDescriptorSets(m_Driver->GetContext()->Device(), &allocInfo, &descriptorSet),
                         "Descripto Set Creation");

                std::vector<VkWriteDescriptorSet>   writes(setDescriptor.bindings.size());
                std::vector<VkDescriptorImageInfo>  images(setDescriptor.bindings.size());
                std::vector<VkDescriptorBufferInfo> buffers(setDescriptor.bindings.size());

                for (uint32_t binding = 0; binding < setDescriptor.bindings.size(); binding++)
                {
                    auto& write           = writes[binding];
                    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.dstSet          = descriptorSet;
                    write.dstBinding      = binding;
                    write.dstArrayElement = 0;
                    write.descriptorCount = 1;

                    auto bindingHandle   = m_CurrentBinding.set[set][binding].handle;
                    auto bindingType     = m_CurrentBinding.set[set][binding].type;
                    write.descriptorType = VulkanUtil::GetDescriptorType(bindingType);

                    VkDescriptorImageInfo&  image  = images[binding];
                    VkDescriptorBufferInfo& buffer = buffers[binding];

                    switch (bindingType)
                    {
                        case DescriptorType::CombinedImageSampler:
                            image.imageView   = m_Driver->GetTexture(bindingHandle)->GetMainView();
                            image.imageLayout = m_Driver->GetTexture(bindingHandle)->GetUsage() &
                                                        TextureUsageBits::DepthStencilAttachment ?
                                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            image.sampler     = m_Driver->GetSampler();
                            write.pImageInfo  = &image;
                            break;
                        case DescriptorType::StorageImage:
                            image.imageView   = m_Driver->GetTexture(bindingHandle)->GetMainView();
                            image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                            write.pImageInfo  = &image;
                            break;
                        case DescriptorType::StorageBuffer:
                        case DescriptorType::StorageBufferDynamic:
                        case DescriptorType::UniformBuffer:
                        case DescriptorType::UniformBufferDynamic:
                            buffer.buffer     = m_Driver->GetBuffer(bindingHandle)->GetBuffer();
                            buffer.offset     = 0;
                            buffer.range      = m_Driver->GetBuffer(bindingHandle)->GetRange();
                            write.pBufferInfo = &buffer;
                            break;
                        default:
                            assert(false);
                    }
                }

                vkUpdateDescriptorSets(m_Driver->GetContext()->Device(), writes.size(), writes.data(), 0, nullptr);

                vkCmdBindDescriptorSets(cb,
                                        bindPoint,
                                        m_BoundShader->GetPipelineLayout(),
                                        set,
                                        1,
                                        &descriptorSet,
                                        offsets.size(),
                                        offsets.data());
                DescriptorState s {m_DescriptorSetCache.size(), true};

                m_PipelineDescriptorCache[pipeline][set].insert({cacheKey, s});
                m_DescriptorSetCache.push_back(descriptorSet);
            }
        }
    }

    void VulkanPipelineCache::Reset()
    {
        m_BoundShader     = nullptr;
        m_BoundRenderPass = nullptr;
        m_BoundPushConstantList.clear();
        m_CurrentBoundPipelineGraphics = VK_NULL_HANDLE;
        m_CurrentBoundPipelineCompute  = VK_NULL_HANDLE;
        m_FreshPipelineGraphics        = true;
        m_FreshPipelineCompute         = true;

        for (auto& cache : m_PipelineDescriptorCache)
        {
            for (auto& set : cache.second)
            {
                for (auto& key : set)
                {
                    key.second.bound = false;
                }
            }
        }
    }

    VkPipeline VulkanPipelineCache::CreatePipelineGraphics()
    {
        PipelineCacheKey currentKey = {m_BoundRenderPass->GetDescriptor(), m_BoundShader, m_CurrentRasterState};

        auto iter = m_PipelineCacheGraphics.find(currentKey);
        if (iter != m_PipelineCacheGraphics.end())
        {
            return iter->second;
        }

        auto context = m_Driver->GetContext();

        VkVertexInputBindingDescription vibd {};
        vibd.binding   = 0;
        vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vibd.stride    = 56;

        VkVertexInputAttributeDescription vabd[5];
        // position vec3
        vabd[0].binding  = 0;
        vabd[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        vabd[0].location = 0;
        vabd[0].offset   = 0;
        // normal vec3
        vabd[1].binding  = 0;
        vabd[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        vabd[1].location = 1;
        vabd[1].offset   = 12;
        // tangent vec3
        vabd[2].binding  = 0;
        vabd[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        vabd[2].location = 2;
        vabd[2].offset   = 24;
        // bitangent
        vabd[3].binding  = 0;
        vabd[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
        vabd[3].location = 3;
        vabd[3].offset   = 36;
        // uv
        vabd[4].binding  = 0;
        vabd[4].format   = VK_FORMAT_R32G32_SFLOAT;
        vabd[4].location = 4;
        vabd[4].offset   = 48;

        VkPipelineVertexInputStateCreateInfo vertexInput {};
        vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount   = 1;
        vertexInput.pVertexBindingDescriptions      = &vibd;
        vertexInput.vertexAttributeDescriptionCount = sizeof(vabd) / sizeof(vabd[0]);
        vertexInput.pVertexAttributeDescriptions    = vabd;

        // right now only support triangle strip
        VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
        inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineTessellationStateCreateInfo tes {};
        tes.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

        // doesn't matter. we use dynamic viewport and scissor
        VkViewport viewport {};
        VkRect2D   scissor {};

        VkPipelineViewportStateCreateInfo viewportScissor {};
        viewportScissor.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportScissor.viewportCount = 1;
        viewportScissor.scissorCount  = 1;
        viewportScissor.pViewports    = &viewport;
        viewportScissor.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo raster {};
        raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.cullMode    = VulkanUtil::GetCullMode(m_CurrentRasterState.cull);
        raster.frontFace   = VulkanUtil::GetFrontFace(m_CurrentRasterState.frontFace);
        raster.lineWidth   = 1.f;
        raster.polygonMode = VK_POLYGON_MODE_FILL;

        // we currently dont support multisample
        VkPipelineMultisampleStateCreateInfo multisample {};
        multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // we currently dont support stencil test
        VkPipelineDepthStencilStateCreateInfo depthStencil {};
        depthStencil.sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthWriteEnable  = m_CurrentRasterState.depthWriteEnabled ? VK_TRUE : VK_FALSE;
        depthStencil.depthTestEnable   = m_CurrentRasterState.depthTestEnabled ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp    = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.stencilTestEnable = VK_FALSE;

        // default color blending option
        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        colorBlendAttachment.blendEnable         = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending {};
        colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments    = &colorBlendAttachment;

        // viewport and scissor
        VkDynamicState states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamic {};
        dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates    = states;

        VkGraphicsPipelineCreateInfo createInfo {};
        createInfo.sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.stageCount        = m_BoundShader->GetShaderStages().size();
        createInfo.pStages           = m_BoundShader->GetShaderStages().data();
        createInfo.pVertexInputState = m_BoundShader->GetVertexInputLayout();
        // createInfo.pVertexInputState   = &vertexInput;
        createInfo.pInputAssemblyState = &inputAssembly;
        createInfo.pTessellationState  = &tes;
        createInfo.pViewportState      = &viewportScissor;
        createInfo.pRasterizationState = &raster;
        createInfo.pMultisampleState   = &multisample;
        createInfo.pDepthStencilState  = &depthStencil;
        createInfo.pColorBlendState    = &colorBlending;
        createInfo.pDynamicState       = &dynamic;
        createInfo.layout              = m_BoundShader->GetPipelineLayout();
        createInfo.renderPass          = m_BoundRenderPass->GetRenderPass();
        createInfo.subpass             = 0;

        VkPipeline pip;

        VK_CHECK(vkCreateGraphicsPipelines(context->Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pip),
                 "Graphics Pipeline Creation");
        m_PipelineCacheGraphics.insert({currentKey, pip});

        return pip;
    }
    VkPipeline VulkanPipelineCache::CreatePipelineCompute()
    {
        assert(m_BoundShader);
        assert(m_BoundShader->GetPipelineType() == PipelineTypeBits::Compute);

        auto iter = m_PipelineCacheCompute.find(m_BoundShader);
        if (iter != m_PipelineCacheCompute.end())
        {
            return iter->second;
        }

        auto shaderModule = m_BoundShader->GetShaderModules();
        assert(shaderModule.size() == 1);

        VkComputePipelineCreateInfo createInfo {};
        createInfo.sType        = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        createInfo.stage.module = shaderModule[0];
        createInfo.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        createInfo.stage.pName  = "main";
        createInfo.layout       = m_BoundShader->GetPipelineLayout();

        VkPipeline pipelineCompute;
        VK_CHECK(vkCreateComputePipelines(
                     m_Driver->GetContext()->Device(), nullptr, 1, &createInfo, nullptr, &pipelineCompute),
                 "Compute pipeline creation");

        m_PipelineCacheCompute.insert({m_BoundShader, pipelineCompute});

        return pipelineCompute;
    }
} // namespace Zephyr