#include "VulkanShaderSet.h"
#include "VulkanContext.h"
#include "VulkanDriver.h"
#include "VulkanUtil.h"
#include <spirv_cross/spirv_cross.hpp>

namespace Zephyr
{
    VulkanShaderSet::VulkanShaderSet(VulkanDriver* driver, const ShaderSetDescription& desc) :
        m_PipelineType(desc.pipeline)
    {

        if (desc.pipeline == PipelineTypeBits::Graphics)
        {
            Reflect(driver, desc.vertex, ShaderStageBits::Vertex);
            Reflect(driver, desc.fragment, ShaderStageBits::Fragment);
            Compile(driver, {desc.vertex, desc.fragment});
            SetupVertexLayout(desc.vertexType);
            assert(m_PipelineLayoutDescriptor.Validate());
        }
        else if (desc.pipeline == PipelineTypeBits::Compute)
        {
            Reflect(driver, desc.compute, ShaderStageBits::Compute);
            Compile(driver, {desc.compute});
            assert(m_PipelineLayoutDescriptor.Validate());
        }
    }

    VulkanShaderSet::~VulkanShaderSet() {}

    void VulkanShaderSet::Destroy(VulkanDriver* driver)
    {
        auto& context = *driver->GetContext();
        for (auto& module : m_ShaderModules)
        {
            vkDestroyShaderModule(context.Device(), module, nullptr);
        }

        for (auto& layout : m_DescriptorLayouts)
        {
            vkDestroyDescriptorSetLayout(context.Device(), layout, nullptr);
        }

        vkDestroyPipelineLayout(context.Device(), m_PipelineLayout, nullptr);
    }

    void VulkanShaderSet::Reflect(VulkanDriver* driver, const std::vector<uint32_t>& data, ShaderStage stage)
    {
        auto&                     context = *driver->GetContext();
        spirv_cross::CompilerGLSL compiler(data.data(), data.size());
        auto&                     resource = compiler.get_shader_resources();

        // we need these information to create a pipeline layout
        // 1. set and binding 2. descriptor count(if the descriptor is an array)
        // 3. descritor type(uniform buffer/ combine image sampler) note that cubemap/sampler2d/sampler2darray/... are
        // all of type sampler
        // 4. shader stages
        // for push constant, we also needs its size and stride
        for (auto& uniform : resource.uniform_buffers)
        {
            const spirv_cross::SPIRType& type    = compiler.get_type(uniform.type_id);
            auto                         set     = compiler.get_decoration(uniform.id, spv::DecorationDescriptorSet);
            auto                         binding = compiler.get_decoration(uniform.id, spv::DecorationBinding);
            uint32_t                     size    = 1;
            // right now we only support 1D array
            if (type.array.size() > 0)
            {
                size = type.array[0];
            }

            m_PipelineLayoutDescriptor.Set(set, binding, size, DescriptorType::UniformBufferDynamic, stage);
        }
        // assume that storage buffer at set 0 binding 0 is dynamic. need to find a more flexible way
        for (auto& ssbo : resource.storage_buffers)
        {
            const spirv_cross::SPIRType& type    = compiler.get_type(ssbo.type_id);
            auto                         set     = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
            auto                         binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
            uint32_t                     size    = 1;
            if (type.array.size() > 0)
            {
                size = type.array[0];
            }

            auto descType = DescriptorType::StorageBuffer;
            if (set == 0 && binding == 0)
            {
                descType = DescriptorType::StorageBufferDynamic;
            }

            m_PipelineLayoutDescriptor.Set(set, binding, size, descType, stage);
        }

        // right now we do not support separate image and sampler
        for (auto& sampler : resource.sampled_images)
        {
            const spirv_cross::SPIRType& type    = compiler.get_type(sampler.type_id);
            auto                         set     = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
            auto                         binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
            uint32_t                     size    = 1;

            if (type.array.size() > 0)
            {
                size = type.array[0];
            }

            m_PipelineLayoutDescriptor.Set(set, binding, size, DescriptorType::CombinedImageSampler, stage);
        }
        // storage image is only write to in compute shader
        for (auto& storageImage : resource.storage_images)
        {
            const spirv_cross::SPIRType& type = compiler.get_type(storageImage.type_id);
            auto                         set  = compiler.get_decoration(storageImage.id, spv::DecorationDescriptorSet);
            auto                         binding = compiler.get_decoration(storageImage.id, spv::DecorationBinding);
            uint32_t                     size    = 1;

            if (type.array.size() > 0)
            {
                size = type.array[0];
            }

            m_PipelineLayoutDescriptor.Set(set, binding, size, DescriptorType::StorageImage, stage);
        }

        for (auto& pushConstant : resource.push_constant_buffers)
        {
            uint32_t offset = 0;
            uint32_t size   = 0;

            // this gives info about every member in the push constant.
            // we only care about the initial offset and the total range
            auto& ranges = compiler.get_active_buffer_ranges(pushConstant.id);
            for (auto& range : ranges)
            {
                if (range.index == 0)
                {
                    offset = range.offset;
                }
                size += range.range;
            }
            // assert(size != 0);
            if (size != 0)
            {
                m_PipelineLayoutDescriptor.Set(size, offset, stage);
            }
        }
    }

    void VulkanShaderSet::Compile(VulkanDriver* driver, const std::vector<std::vector<uint32_t>>& shaderCode)
    {
        auto& context = *driver->GetContext();
        // create descriptor layout
        for (auto& setDescriptor : m_PipelineLayoutDescriptor.layouts)
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            for (uint32_t i = 0; i < setDescriptor.bindings.size(); i++)
            {
                auto& bindingDescritor = setDescriptor.bindings[i];

                auto& binding           = bindings.emplace_back();
                binding.binding         = i;
                binding.descriptorCount = bindingDescritor.size;
                binding.descriptorType  = VulkanUtil::GetDescriptorType(bindingDescritor.type);
                binding.stageFlags      = VulkanUtil::GetShaderStageFlags(bindingDescritor.stages);
            }

            VkDescriptorSetLayoutCreateInfo createInfo {};
            createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = bindings.size();
            createInfo.pBindings    = bindings.data();

            auto& descriptorLayout = m_DescriptorLayouts.emplace_back();

            VK_CHECK(vkCreateDescriptorSetLayout(context.Device(), &createInfo, nullptr, &descriptorLayout),
                     "Descriptor Layout Creation");
        }
        // create pipeline layout
        {
            std::vector<VkPushConstantRange> pushConstantRanges;
            for (auto& rangeDescriptor : m_PipelineLayoutDescriptor.pushConstants)
            {
                auto& range      = pushConstantRanges.emplace_back();
                range.offset     = rangeDescriptor.offset;
                range.size       = rangeDescriptor.size;
                range.stageFlags = VulkanUtil::GetShaderStageFlags(rangeDescriptor.stages);
            }

            VkPipelineLayoutCreateInfo createInfo {};
            createInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            createInfo.setLayoutCount         = m_DescriptorLayouts.size();
            createInfo.pSetLayouts            = m_DescriptorLayouts.data();
            createInfo.pushConstantRangeCount = pushConstantRanges.size();
            createInfo.pPushConstantRanges    = pushConstantRanges.data();

            VK_CHECK(vkCreatePipelineLayout(context.Device(), &createInfo, nullptr, &m_PipelineLayout),
                     "Pipeline Layout Creation");
        }
        // create shader module
        if (m_PipelineType == PipelineTypeBits::Graphics)
        {
            assert(shaderCode.size() == 2);

            VkShaderModuleCreateInfo createInfo {};
            createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = shaderCode[0].size() * 4;
            createInfo.pCode    = shaderCode[0].data();

            VkShaderModule shaderModuleVertex;
            VK_CHECK(vkCreateShaderModule(context.Device(), &createInfo, nullptr, &shaderModuleVertex),
                     "Vertex Shader Creation");

            createInfo.codeSize = shaderCode[1].size() * 4;
            createInfo.pCode    = shaderCode[1].data();

            VkShaderModule shaderModuleFragment;
            VK_CHECK(vkCreateShaderModule(context.Device(), &createInfo, nullptr, &shaderModuleFragment),
                     "Frament Shader Creation");

            m_ShaderStages.resize(2);
            m_ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            m_ShaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
            m_ShaderStages[0].pName  = "main";
            m_ShaderStages[0].module = shaderModuleVertex;

            m_ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            m_ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            m_ShaderStages[1].pName  = "main";
            m_ShaderStages[1].module = shaderModuleFragment;

            m_ShaderModules.push_back(shaderModuleVertex);
            m_ShaderModules.push_back(shaderModuleFragment);
        }
        else if (m_PipelineType == PipelineTypeBits::Compute)
        {
            assert(shaderCode.size() == 1);

            VkShaderModuleCreateInfo createInfo {};
            createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = shaderCode[0].size() * 4;
            createInfo.pCode    = shaderCode[0].data();

            auto& shaderModuleCompute = m_ShaderModules.emplace_back();
            VK_CHECK(vkCreateShaderModule(context.Device(), &createInfo, nullptr, &shaderModuleCompute),
                     "Compute Shader Creation");
            m_ShaderStages.resize(1);
            m_ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            m_ShaderStages[0].stage  = VK_SHADER_STAGE_COMPUTE_BIT;
            m_ShaderStages[0].pName  = "main";
            m_ShaderStages[0].module = shaderModuleCompute;
        }
        else
        {
            assert(false);
        }
    }
    void VulkanShaderSet::SetupVertexLayout(VertexType type)
    {
        static VkVertexInputBindingDescription vibd {};
        vibd.binding   = 0;
        vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vibd.stride    = 56;

        static VkVertexInputAttributeDescription vabd[5];
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

        static VkPipelineVertexInputStateCreateInfo staticVertexInput {};
        staticVertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        staticVertexInput.vertexBindingDescriptionCount   = 1;
        staticVertexInput.pVertexBindingDescriptions      = &vibd;
        staticVertexInput.vertexAttributeDescriptionCount = sizeof(vabd) / sizeof(vabd[0]);
        staticVertexInput.pVertexAttributeDescriptions    = vabd;

        static VkPipelineVertexInputStateCreateInfo emptyVertexInput {};
        emptyVertexInput.sType                            = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        emptyVertexInput.vertexBindingDescriptionCount    = 0;
        emptyVertexInput.pVertexBindingDescriptions       = nullptr;
        emptyVertexInput.vertexAttributeDescriptionCount  = 0;
        emptyVertexInput.pVertexAttributeDescriptions     = nullptr;

        switch (type)
        {
            case VertexType::None:
                m_VertexState = emptyVertexInput;
                break;
            case VertexType::Static:
                m_VertexState = staticVertexInput;
                break;
            case VertexType::Dynamic:
                assert(false);
                break;
            default:
                assert(false);
        }
    }
} // namespace Zephyr