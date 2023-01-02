#pragma once
#include "pch.h"
#include "VulkanCommon.h"
#include "rhi/RHIEnums.h"

namespace Zephyr
{
    class VulkanDriver;
    class VulkanSamplerCache final
    {
    public:
        VulkanSamplerCache(VulkanDriver* driver);
        ~VulkanSamplerCache();

        void Shutdown();

        VkSampler GetSampler(SamplerWrap addressMode);

    private:
        VulkanDriver* m_Driver;
        std::unordered_map<SamplerWrap, VkSampler> m_SamplerCache;
    };
}