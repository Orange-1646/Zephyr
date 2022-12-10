#pragma once
#include "VulkanCommon.h"
#include <iostream>

namespace Zephyr
{

    class VulkanValidation
    {
    public:
        VulkanValidation();
        ~VulkanValidation();

        bool Init(VkInstance instace);

    public:
        VkInstance                         m_Instance;
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {};
        VkDebugUtilsMessengerEXT           callback;

        bool m_Initialized = false;
    };
} // namespace Cosmo