#pragma once
#include "VulkanCommon.h"
#include "rhi/RHITexture.h"

namespace Zephyr
{
    class VulkanDriver;
    class VulkanContext;

    class VulkanTexture : public RHITexture
    {
    public:
        // general texture
        VulkanTexture(VulkanDriver* driver, const TextureDescription& desc);
        // texture for swapchain images. In this case texture doesn't own the image
        // image will not be destroyed on destruction
        VulkanTexture(VulkanDriver* driver, const TextureDescription& desc, VkImage image, VkFormat format);

        ~VulkanTexture() override = default;

        TextureFormat GetFormat() override;

        void Destroy(VulkanDriver* driver);
        void GenerateMips(VulkanDriver* driver);

        void Update(VulkanDriver* driver, const TextureUpdateDescriptor& desc);
        void TransitionLayout(VkCommandBuffer cb,
                              uint32_t        depth,
                              uint32_t        layer,
                              uint32_t        layerCount,
                              uint32_t        level,
                              uint32_t        levelCount,
                              VkImageLayout   target,
                              PipelineType    pipeline = PipelineTypeBits::None);

        // track the layout of target range
        void SetLayout(uint32_t layer, uint32_t level, VkImageLayout layout);

        // get imageview
        VkImageView        GetView(VulkanDriver* driver, const ViewRange& range);
        inline VkImageView GetMainView() { return m_MainView; }

        inline std::pair<uint32_t, uint32_t> GetDimension() const
        {
            return {m_Description.width, m_Description.height};
        }
        inline TextureUsage GetUsage() const { return m_Description.usage; }
        inline VkImage      GetImage() const { return m_Image; }

    private:
        VkImageLayout GetLayout(uint32_t layer, uint32_t level);
        void          CreateDefaultImageView(VulkanDriver* driver);

        VkImageView        CreateImageView(VulkanDriver* driver, const ViewRange& range);
        VkImageAspectFlags GetAspect();
        uint32_t           GetLayerCount();

    private:
        bool               m_IsExternalImage = false;
        VkFormat           m_ExternalImageFormat;
        TextureDescription m_Description;
        VkImage            m_Image  = VK_NULL_HANDLE;
        VkDeviceMemory     m_Memory = VK_NULL_HANDLE;

        ViewRange   m_MainViewRange;
        VkImageView m_MainView = VK_NULL_HANDLE;

        struct pair_hash
        {
            template<class T1, class T2>
            std::size_t operator()(const std::pair<T1, T2>& pair) const
            {
                return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
            }
        };

        std::unordered_map<ViewRange, VkImageView, ViewRangeHasher>                 m_ViewCache;
        std::unordered_map<std::pair<uint32_t, uint32_t>, VkImageLayout, pair_hash> m_Layouts;
    };
} // namespace Zephyr