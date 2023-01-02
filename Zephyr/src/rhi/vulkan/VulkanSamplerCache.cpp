#include "VulkanSamplerCache.h"
#include "VulkanDriver.h"
#include "VulkanUtil.h"

namespace Zephyr
{
    VulkanSamplerCache::VulkanSamplerCache(VulkanDriver* driver) : m_Driver(driver) {}
    VulkanSamplerCache::~VulkanSamplerCache() {}
    void VulkanSamplerCache::Shutdown()
    {
        for (auto& sampler : m_SamplerCache)
        {
            vkDestroySampler(m_Driver->GetContext()->Device(), sampler.second, nullptr);
        }
    }
    VkSampler VulkanSamplerCache::GetSampler(SamplerWrap addressMode)
    {
        auto iter = m_SamplerCache.find(addressMode);

        if (iter != m_SamplerCache.end())
        {
            return iter->second;
        }
        auto mode = VulkanUtil::GetAddressMode(addressMode);

        VkSamplerCreateInfo createInfo {};
        createInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.addressModeU  = mode;
        createInfo.addressModeV  = mode;
        createInfo.addressModeW  = mode;
        createInfo.minFilter     = VK_FILTER_LINEAR;
        createInfo.magFilter     = VK_FILTER_LINEAR;
        createInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.maxLod        = 10;
        createInfo.compareEnable = VK_TRUE;
        createInfo.compareOp     = VK_COMPARE_OP_LESS;

        VkSampler sampler;

        VK_CHECK(vkCreateSampler(m_Driver->GetContext()->Device(), &createInfo, nullptr, &sampler), "Sampler Creation");

        m_SamplerCache.insert({addressMode, sampler});

        return sampler;
    }
} // namespace Zephyr