#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDriver.h"
#include "VulkanUtil.h"

namespace Zephyr
{
    VulkanTexture::VulkanTexture(VulkanDriver* driver, const TextureDescription& desc) : m_Description(desc)
    {
        auto& context = *driver->GetContext();
        // create image
        {
            std::vector<uint32_t> queueFamilyIndices;
            if (desc.pipelines & PipelineTypeBits::Graphics)
            {
                queueFamilyIndices.push_back(context.QueueIndices().graphics);
            }
            if (desc.pipelines & PipelineTypeBits::Compute)
            {
                queueFamilyIndices.push_back(context.QueueIndices().compute);
            }

            VkImageCreateInfo createInfo {};
            createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            createInfo.imageType     = desc.sampler == SamplerType::Sampler3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
            createInfo.format        = VulkanUtil::GetTextureFormat(desc.format);
            createInfo.extent.width  = desc.width;
            createInfo.extent.height = desc.height;
            createInfo.extent.depth  = desc.depth;
            createInfo.mipLevels     = desc.levels;
            createInfo.arrayLayers   = 1;
            // TODO: add multisample support
            createInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
            createInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage                 = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            createInfo.sharingMode           = VulkanUtil::GetSharingMode(desc.pipelines);
            createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
            createInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

            if (desc.sampler == SamplerType::Sampler2DArray)
            {
                createInfo.extent.depth = 1;
                createInfo.arrayLayers  = desc.depth;
            }
            if (desc.sampler == SamplerType::SamplerCubeMap)
            {
                createInfo.arrayLayers = 6;
                createInfo.flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }
            if (desc.sampler == SamplerType::SamplerCubeMapArray)
            {
                createInfo.arrayLayers  = 6 * desc.depth;
                createInfo.extent.depth = 1;
            }

            if (desc.usage & TextureUsageBits::ColorAttachment)
            {
                createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if (desc.usage & TextureUsageBits::DepthStencilAttachment)
            {
                createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            if (desc.usage & TextureUsageBits::Sampled)
            {
                createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if (desc.usage & TextureUsageBits::Storage)
            {
                createInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            }

            VK_CHECK(vkCreateImage(context.Device(), &createInfo, nullptr, &m_Image), "Image Creation");
        }

        // allocate and bind memory
        {
            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(context.Device(), m_Image, &memoryRequirements);

            uint32_t requiredSize = memoryRequirements.size;
            uint32_t memoryTypeIndex =
                VulkanUtil::FindMemoryTypeIndex(context.PhysicalDevice(),
                                                memoryRequirements.memoryTypeBits,
                                                VulkanUtil::GetBufferMemoryProperties(BufferMemoryType::Static));

            VkMemoryAllocateInfo allocInfo {};
            allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize  = requiredSize;
            allocInfo.memoryTypeIndex = memoryTypeIndex;

            VK_CHECK(vkAllocateMemory(context.Device(), &allocInfo, nullptr, &m_Memory), "Texture Memory Allocation");

            vkBindImageMemory(context.Device(), m_Image, m_Memory, 0);
        }
        // create main image view
        CreateDefaultImageView(driver);
    }

    VulkanTexture::VulkanTexture(VulkanDriver*             driver,
                                 const TextureDescription& desc,
                                 VkImage                   swapchainImage,
                                 VkFormat                  format) :
        m_Image(swapchainImage),
        m_Description(desc), m_IsExternalImage(true), m_ExternalImageFormat(format)
    {
        CreateDefaultImageView(driver);
    }

    TextureFormat VulkanTexture::GetFormat() { return m_Description.format; }

    void VulkanTexture::Destroy(VulkanDriver* driver)
    {
        auto& context = *driver->GetContext();
        for (auto& view : m_ViewCache)
        {
            vkDestroyImageView(context.Device(), view.second, nullptr);
        }

        if (!m_IsExternalImage)
        {
            vkFreeMemory(context.Device(), m_Memory, nullptr);
            vkDestroyImage(context.Device(), m_Image, nullptr);
        }
    }

    // TODO: support 3D texture
    void VulkanTexture::Update(VulkanDriver* driver, const TextureUpdateDescriptor& desc)
    {
        // create staging buffer
        BufferDescription stageDesc {};
        stageDesc.memoryType = BufferMemoryType::Dynamic;
        stageDesc.pipelines  = m_Description.pipelines;
        stageDesc.size       = desc.size;
        stageDesc.usage      = BufferUsageBits::None;

        VulkanBuffer stage(driver, stageDesc);
        stage.Update(driver, {0, 0, desc.data, desc.size});

        VkBufferImageCopy copy {};
        copy.bufferOffset                    = 0;
        copy.bufferRowLength                 = 0;
        copy.bufferImageHeight               = 0;
        copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.baseArrayLayer = desc.layer;
        copy.imageSubresource.layerCount     = 1;
        copy.imageSubresource.mipLevel       = desc.level;
        copy.imageOffset.x                   = 0;
        copy.imageOffset.y                   = 0;
        copy.imageOffset.z                   = 0;
        copy.imageExtent.width               = m_Description.width;
        copy.imageExtent.height              = m_Description.height;
        copy.imageExtent.depth               = desc.depth;

        auto cb = driver->BeginSingleTimeCommandBuffer();
        TransitionLayout(cb, 0, desc.layer, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdCopyBufferToImage(cb, stage.m_Buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        TransitionLayout(cb, 0, desc.layer, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        driver->EndSingleTimeCommandBuffer(cb);
        stage.Destroy(driver);
    }

    void VulkanTexture::TransitionLayout(VkCommandBuffer cb,
                                         uint32_t        depth,
                                         uint32_t        layer,
                                         uint32_t        layerCount,
                                         VkImageLayout   target)
    {
        auto transition = VulkanUtil::GetImageTransitionMask(target);
        auto lc         = layerCount == ALL_LAYERS ? m_Description.depth - layer : layerCount;

        std::vector<VkImageMemoryBarrier> barriers;
        barriers.reserve(lc);

        for (uint32_t i = layer; i < lc + layer; i++)
        {
            auto layout = GetLayout(depth, i);
            if (layout != target)
            {
                auto& barrier                           = barriers.emplace_back();
                barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout                       = GetLayout(depth, layer);
                barrier.newLayout                       = target;
                barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                barrier.image                           = m_Image;
                barrier.subresourceRange.aspectMask     = VulkanUtil::GetAspectMaskFromUsage(m_Description.usage);
                barrier.subresourceRange.baseArrayLayer = i;
                barrier.subresourceRange.layerCount     = 1;
                barrier.subresourceRange.baseMipLevel   = 0;
                barrier.subresourceRange.levelCount     = m_Description.levels;
                barrier.srcAccessMask                   = transition.srcAccessMask;
                barrier.dstAccessMask                   = transition.dstAccessMask;
            }
        }
        // VkImageMemoryBarrier barrier            = {};
        // barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        // barrier.oldLayout                       = GetLayout(depth, layer);
        // barrier.newLayout                       = target;
        // barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        // barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        // barrier.image                           = m_Image;
        // barrier.subresourceRange.aspectMask     = VulkanUtil::GetAspectMaskFromUsage(m_Description.usage);
        // barrier.subresourceRange.baseArrayLayer = layer;
        // barrier.subresourceRange.layerCount     = lc;
        // barrier.subresourceRange.baseMipLevel   = 0;
        // barrier.subresourceRange.levelCount     = m_Description.levels;
        // barrier.srcAccessMask                   = transition.srcAccessMask;
        // barrier.dstAccessMask                   = transition.dstAccessMask;

        vkCmdPipelineBarrier(
            cb, transition.srcStage, transition.dstStage, 0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());

        for (uint32_t i = layer; i < lc + layer; i++)
        {
            SetLayout(depth, i, target);
        }
    }

    void VulkanTexture::SetLayout(uint32_t depth, uint32_t layer, VkImageLayout layout)
    {
        auto iter = m_Layouts.find({depth, layer});
        if (iter != m_Layouts.end())
        {
            iter->second = layout;
        }
        else
        {
            m_Layouts.insert({{depth, layer}, layout});
        }
    }

    VkImageView VulkanTexture::GetView(VulkanDriver* driver, const ViewRange& range)
    {
        return CreateImageView(driver, range);
    }

    VkImageLayout VulkanTexture::GetLayout(uint32_t depth, uint32_t layer)
    {
        auto iter = m_Layouts.find({depth, layer});
        if (iter == m_Layouts.end())
        {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
        return iter->second;
    }

    void VulkanTexture::CreateDefaultImageView(VulkanDriver* driver)
    {
        m_MainViewRange.baseLayer  = 0;
        m_MainViewRange.layerCount = 1;
        if (m_Description.sampler == SamplerType::Sampler2DArray)
        {
            m_MainViewRange.layerCount = m_Description.depth;
        }
        else if (m_Description.sampler == SamplerType::SamplerCubeMap)
        {
            m_MainViewRange.layerCount = 6;
        }
        else if (m_Description.sampler == SamplerType::SamplerCubeMapArray)
        {
            m_MainViewRange.layerCount = 6 * m_Description.depth;
        }
        m_MainViewRange.baseLevel  = 0;
        m_MainViewRange.levelCount = m_Description.levels;

        m_MainView = CreateImageView(driver, m_MainViewRange);
    }

    VkImageView VulkanTexture::CreateImageView(VulkanDriver* driver, const ViewRange& range)
    {
        auto& context = *driver->GetContext();
        auto  iter    = m_ViewCache.find(range);
        if (iter != m_ViewCache.end())
        {
            return iter->second;
        }

        VkImageViewCreateInfo createInfo {};
        createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                           = m_Image;
        createInfo.viewType                        = VulkanUtil::GetImageViewType(m_Description.sampler);
        createInfo.format                          = m_Description.format == TextureFormat::DEFAULT ?
                                                         m_ExternalImageFormat :
                                                         VulkanUtil::GetTextureFormat(m_Description.format);
        createInfo.components                      = {VK_COMPONENT_SWIZZLE_IDENTITY,
                                 VK_COMPONENT_SWIZZLE_IDENTITY,
                                 VK_COMPONENT_SWIZZLE_IDENTITY,
                                 VK_COMPONENT_SWIZZLE_IDENTITY};
        createInfo.subresourceRange.aspectMask     = GetAspect();
        createInfo.subresourceRange.baseArrayLayer = range.baseLayer;
        createInfo.subresourceRange.layerCount     = range.layerCount;
        createInfo.subresourceRange.baseMipLevel   = range.baseLevel;
        createInfo.subresourceRange.levelCount     = range.levelCount;

        VkImageView view;

        VK_CHECK(vkCreateImageView(context.Device(), &createInfo, nullptr, &view), "Image View Creation");

        m_ViewCache.insert({range, view});

        return view;
    }

    VkImageAspectFlags VulkanTexture::GetAspect()
    {
        return VulkanUtil::IsDepthFormat(m_Description.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    }
} // namespace Zephyr