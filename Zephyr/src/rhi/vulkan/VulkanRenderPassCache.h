#pragma once
#include "VulkanCommon.h"


namespace Zephyr
{
    // like VulkanPipelineCache, VulkanRenderPassCache caches all renderpass created during framegraph compilation
    class VulkanRenderPassCache final
    {
    public:
        VulkanRenderPassCache();
        ~VulkanRenderPassCache();
    };
}