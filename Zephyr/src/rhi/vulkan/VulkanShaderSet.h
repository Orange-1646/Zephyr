#pragma once
#include "VulkanCommon.h"
#include "pch.h"
#include "rhi/RHIShaderSet.h"

#include <spirv_glsl.hpp>

namespace Zephyr
{
    /*
        A VulkanShaderSet represents a combination of executable shaders.
        Descriptor layouts and pipeline layouts are automatically generated upon spirv loading
    */
    class VulkanContext;
    class VulkanDriver;

    struct BindingInfo
    {
        DescriptorType type;
        uint32_t       size;
        ShaderStage    stages;
    };

    struct DescriptorSetLayout
    {
        std::vector<BindingInfo> bindings;
    };

    struct PushConstantLayout
    {
        uint32_t    size;
        uint32_t    offset;
        ShaderStage stages;

        bool operator==(const PushConstantLayout& rhs) { return rhs.size == size && rhs.offset == offset; }
    };

    struct PipelineLayoutDescriptor
    {
        std::vector<DescriptorSetLayout> layouts;
        std::vector<PushConstantLayout>  pushConstants;

        // push constant
        void Set(uint32_t size, uint32_t offset, ShaderStage stages)
        {
            PushConstantLayout l {size, offset, stages};

            for (auto& layout : pushConstants)
            {
                if (layout == l)
                {
                    layout.stages |= stages;
                    return;
                }
            }
            pushConstants.emplace_back(std::move(l));
        }
        // normal descriptor
        void Set(uint32_t set, uint32_t binding, uint32_t size, DescriptorType type, ShaderStage stages)
        {
            if (set + 1 > layouts.size())
            {
                layouts.resize(set + 1);
            }
            auto& descriptorLayout = layouts[set];

            if (binding + 1 > descriptorLayout.bindings.size())
            {
                descriptorLayout.bindings.resize(binding + 1);
            }
            auto& descriptorBinding = descriptorLayout.bindings[binding];

            // multiple assignment on the same binding within the same shader stage indicates bug in the shader
            assert(descriptorBinding.stages != stages);

            descriptorBinding.type = type;
            descriptorBinding.stages |= stages;
            descriptorBinding.size = size;
        }

        // TODO: implement
        bool Validate()
        {
            // make sure there's no hole between sets and no hole between bindings
            return true;
        }
    };

    class VulkanShaderSet : public RHIShaderSet
    {
    public:
        VulkanShaderSet(VulkanDriver* driver, const ShaderSetDescription& desc);
        ~VulkanShaderSet() override;

        void                                             Destroy(VulkanDriver* driver);
        inline const std::vector<VkShaderModule>         GetShaderModules() { return m_ShaderModules; }
        inline VkPipelineLayout                          GetPipelineLayout() { return m_PipelineLayout; }
        inline const std::vector<VkDescriptorSetLayout>& GetDescriptorLayouts() { return m_DescriptorLayouts; }
        inline const PipelineLayoutDescriptor& GetPipelineLayoutDescriptor() { return m_PipelineLayoutDescriptor; }
        inline const std::vector<VkPipelineShaderStageCreateInfo>& GetShaderStages() { return m_ShaderStages; }
        inline PipelineType                                        GetPipelineType() const { return m_PipelineType; }
        inline VkPipelineVertexInputStateCreateInfo*               GetVertexInputLayout() { return &m_VertexState; }

    private:
        void Reflect(VulkanDriver* driver, const std::vector<uint32_t>& data, ShaderStage stage);
        void Compile(VulkanDriver* driver, const std::vector<std::vector<uint32_t>>& shaderCode);
        void SetupVertexLayout(VertexType type);

    private:
        PipelineType m_PipelineType;

        // shader reflection info about descriptors
        PipelineLayoutDescriptor m_PipelineLayoutDescriptor;

        // vulkan resource handle
        std::vector<VkDescriptorSetLayout>           m_DescriptorLayouts;
        VkPipelineLayout                             m_PipelineLayout = VK_NULL_HANDLE;
        std::vector<VkShaderModule>                  m_ShaderModules;
        std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
        VkPipelineVertexInputStateCreateInfo           m_VertexState;

        friend class VulkanPipelineCache;
    };
} // namespace Zephyr