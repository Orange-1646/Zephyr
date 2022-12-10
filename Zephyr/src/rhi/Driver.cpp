#include "Driver.h"
#include "rhi/vulkan/VulkanDriver.h"

namespace Zephyr
{
    Driver* Driver::Create(DriverType backend, Window* window) { 
        switch (backend)
        {
            case DriverType::Vulkan:
                return new VulkanDriver(window);
        }

        assert(false);
    }
} // namespace Zephyr::backend