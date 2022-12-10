#pragma once
#include "VulkanCommon.h"
#include "rhi/RHIEnums.h"

namespace Zephyr
{
    struct VulkanTransition
    {
        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;
        VkAccessFlags        srcAccessMask;
        VkAccessFlags        dstAccessMask;
    };

    struct VulkanUtil
    {
        static VkBufferUsageFlags GetVulkanBufferUsage(BufferUsage usage)
        {
            // TODO: would this effect performance?
            VkBufferUsageFlags flag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            if (usage & BufferUsageBits::Index)
            {
                flag |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if (usage & BufferUsageBits::Vertex)
            {
                flag |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if (usage & BufferUsageBits::Uniform)
            {
                flag |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if (usage & BufferUsageBits::UniformDynamic)
            {
                flag |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if (usage & BufferUsageBits::Storage)
            {
                flag |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if (usage & BufferUsageBits::StorageDynamic)
            {
                flag |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }

            return flag;
        }

        static VkSharingMode GetSharingMode(PipelineType pipeline)
        {
            uint32_t pipelineCount = 0;

            if (pipeline & PipelineTypeBits::Graphics)
            {
                pipelineCount++;
            }
            if (pipeline & PipelineTypeBits::Compute)
            {
                pipelineCount++;
            }

            return pipelineCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        }

        static uint32_t
        FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t memoryType, VkMemoryPropertyFlags flags)
        {
            VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

            for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
            {
                if ((memoryType & (1 << i)) != 0 &&
                    (deviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
                {
                    return i;
                }
            }

            assert(!"Failed to find suitable memory type index");
        }

        static VkMemoryPropertyFlags GetBufferMemoryProperties(BufferMemoryType memoryType)
        {
            if (memoryType == BufferMemoryType::Static)
            {
                return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }
            else
            {
                return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
        }

        static VkFormat GetTextureFormat(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::RGBA8_UNORM:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGBA8_SRGB:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case TextureFormat::RGBA16_UNORM:
                    return VK_FORMAT_R16G16B16A16_UNORM;
                case TextureFormat::RGBA16_SRGB:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case TextureFormat::DEPTH32F:
                    return VK_FORMAT_D32_SFLOAT;
                case TextureFormat::DEPTH24_STENCIL8:
                    return VK_FORMAT_D24_UNORM_S8_UINT;
            }
            assert(false);
        }

        static VkImageViewType GetImageViewType(SamplerType sampler)
        {
            switch (sampler)
            {
                case SamplerType::Sampler2D:
                    return VK_IMAGE_VIEW_TYPE_2D;
                case SamplerType::Sampler2DArray:
                    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                case SamplerType::Sampler3D:
                    return VK_IMAGE_VIEW_TYPE_3D;
                case SamplerType::SamplerCubeMap:
                    return VK_IMAGE_VIEW_TYPE_CUBE;
                case SamplerType::SamplerCubeMapArray:
                    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            }

            assert(false);
        }

        static bool IsDepthFormat(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::DEPTH32F:
                case TextureFormat::DEPTH24_STENCIL8:
                    return true;
            }
            return false;
        }

        static VkDescriptorType GetDescriptorType(DescriptorType type)
        {
            switch (type)
            {
                case DescriptorType::UniformBuffer:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case DescriptorType::UniformBufferDynamic:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case DescriptorType::StorageBuffer:
                    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case DescriptorType::StorageBufferDynamic:
                    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                case DescriptorType::CombinedImageSampler:
                    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case DescriptorType::StorageImage:
                    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            }
            // DescriptoType::None and DescriptorType::PushConstant shouldn't get passed here
            assert(false);
        }

        static VkShaderStageFlags GetShaderStageFlags(ShaderStage stages)
        {
            VkShaderStageFlags flag = 0;
            if (stages & ShaderStageBits::Vertex)
            {
                flag |= VK_SHADER_STAGE_VERTEX_BIT;
            }
            if (stages & ShaderStageBits::Fragment)
            {
                flag |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }
            if (stages & ShaderStageBits::Compute)
            {
                flag |= VK_SHADER_STAGE_COMPUTE_BIT;
            }

            assert(flag != 0);

            return flag;
        }

        static VulkanTransition GetImageTransitionMask(VkImageLayout target)
        {
            VulkanTransition transition;
            switch (target)
            {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    transition.srcAccessMask = 0;
                    transition.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    transition.srcStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    transition.dstStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    transition.srcAccessMask = 0;
                    transition.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    transition.srcStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    transition.dstStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                case VK_IMAGE_LAYOUT_GENERAL:
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    transition.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    transition.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    transition.srcStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    transition.dstStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    break;

                    // We support PRESENT as a target layout to allow blitting from the swap chain.
                    // See also SwapChain::makePresentable().
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    transition.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    transition.dstAccessMask = 0;
                    transition.srcStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    transition.dstStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    break;

                default:
                    assert(false);
            }

            return transition;
        }

        static VkCullModeFlags GetCullMode(Culling cull)
        {
            switch (cull)
            {
                case Culling::FrontFace:
                    return VK_CULL_MODE_FRONT_BIT;
                case Culling::BackFace:
                    return VK_CULL_MODE_BACK_BIT;
                case Culling::FrontAndBack:
                    return VK_CULL_MODE_FRONT_AND_BACK;
                case Culling::None:
                    return VK_CULL_MODE_NONE;
            }
            assert(false);
        }

        static VkFrontFace GetFrontFace(FrontFace face)
        {
            switch (face)
            {
                case FrontFace::Clockwise:
                    return VK_FRONT_FACE_CLOCKWISE;
                case FrontFace::CounterClockwise:
                    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            }

            assert(false);
        }

        static VkImageAspectFlags GetAspectMaskFromUsage(TextureUsage usage)
        {
            if (usage & TextureUsageBits::ColorAttachment)
            {
                return VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if (usage & TextureUsageBits::DepthStencilAttachment)
            {
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            if (usage & TextureUsageBits::Sampled)
            {
                return VK_IMAGE_ASPECT_COLOR_BIT;
            }
            assert(false);
        }
        // static VkImageAspectFlags GetAspectMaskFromLayout(VkImageLayout layout)
        //{
        //     switch (layout)
        //     {
        //         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        //             return VK_IMAGE_ASPECT_DEPTH_BIT;
        //         case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        //             return VK
        //     }
        // }

        static VkPipelineStageFlags GetPipelineStageFlagsFromTextureUsage(TextureUsage usage)
        {
            VkPipelineStageFlags flag = 0;
            if (usage & TextureUsageBits::ColorAttachment)
            {
                flag |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            if (usage & TextureUsageBits::DepthStencilAttachment)
            {
                flag |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            }
            if (usage & TextureUsageBits::Storage)
            {
                flag |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }

            return flag;
        }

        static VkAccessFlags GetAccessMaskFromTextureUsage(TextureUsage usage)
        {
            VkAccessFlags flag = 0;
            if (usage & TextureUsageBits::ColorAttachment)
            {
                flag |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (usage & TextureUsageBits::DepthStencilAttachment)
            {
                flag |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            if (usage & TextureUsageBits::Storage)
            {
                flag |= VK_ACCESS_SHADER_WRITE_BIT;
            }
            return flag;
        }

        static VkSemaphore CreateSemaphore(VkDevice device)
        {
            VkSemaphore           sem;
            VkSemaphoreCreateInfo createInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &sem), "Semaphore Creation");

            return sem;
        }
    };
} // namespace Zephyr