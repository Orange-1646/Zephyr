#pragma once
#include "rhi/RHIRenderUnit.h"

namespace Zephyr
{
    class VulkanDriver;
    class VulkanRenderUnit : public RHIRenderUnit
    {
    public:
        VulkanRenderUnit(VulkanDriver* driver, const RenderUnitDescriptor& desc) : m_Descriptor(desc) {}
        ~VulkanRenderUnit() override = default;

        void Destroy(VulkanDriver* driver) {}

    private:
        RenderUnitDescriptor m_Descriptor;
    };
}