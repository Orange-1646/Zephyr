#pragma once
#include "rhi/vulkan/VulkanContext.h"

namespace Zephyr
{
    class GraphicsCore
    {
    public:
        GraphicsCore() = default;
        virtual ~GraphicsCore() = default;

        virtual void Execute() = 0;
    protected:
        VulkanContext m_Context;
    };
}