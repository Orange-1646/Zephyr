#pragma once
#include "VulkanCommon.h"
#include "rhi/RHITexture.h"

namespace Zephyr
{
    constexpr uint32_t ALL_LAYERS = UINT32_MAX;
    class VulkanDriver;
    class VulkanContext;
    struct ViewRange
    {
        uint32_t baseLayer;
        uint32_t layerCount;
        uint32_t baseLevel;
        uint32_t levelCount;

        bool operator==(const ViewRange& rhs) const
        {
            return rhs.baseLayer == baseLayer && rhs.layerCount == layerCount && rhs.baseLayer == baseLevel &&
                   rhs.levelCount == levelCount;
        }
    };

    struct ViewRangeHasher
    {
        std::size_t operator()(const ViewRange& range) const
        {
            using std::hash;
            using std::size_t;
            using std::string;

            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return ((hash<uint32_t>()(range.baseLayer) ^ (hash<uint32_t>()(range.layerCount) << 1)) >> 1) ^
                   (hash<uint32_t>()(range.baseLevel) << 1) ^ (hash<uint32_t>()(range.levelCount) << 1) >> 1;
        }
    };

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

        void Update(VulkanDriver* driver, const TextureUpdateDescriptor& desc);
        void TransitionLayout(VkCommandBuffer cb, uint32_t depth, uint32_t layer, uint32_t layerCount, VkImageLayout target);

        // track the layout of target range
        void SetLayout(uint32_t depth, uint32_t layer, VkImageLayout layout);

        // get imageview
        VkImageView GetView(VulkanDriver* driver, const ViewRange& range);
        inline VkImageView GetMainView() { return m_MainView; }

        inline std::pair<uint32_t, uint32_t> GetDimension() { return {m_Description.width, m_Description.height}; }
        inline TextureUsage                  GetUsage() { return m_Description.usage; }

    private:
        VkImageLayout GetLayout(uint32_t depth, uint32_t layer);
        void          CreateDefaultImageView(VulkanDriver* driver);

        VkImageView        CreateImageView(VulkanDriver* driver, const ViewRange& range);
        VkImageAspectFlags GetAspect();

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