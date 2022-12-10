#include "VulkanValidation.h"

namespace Zephyr
{
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                 void*                                       pUserData)
    {
        static uint32_t c = 0;
        //if (c < 10)
        //{
        //    c++;
        //}
        //else
        //{
        //    return VK_FALSE;
        //}
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            printf("[Validation Layer] Warning: %s\n\n\n\n", pCallbackData->pMessage);
        }

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            printf("[Validation Layer] Error: %s\n\n\n\n", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    VulkanValidation::VulkanValidation()
    {
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        debugMessengerCreateInfo.pfnUserCallback = debugCallback;
    };

    bool VulkanValidation::Init(VkInstance instance)
    {
        m_Instance = instance;
        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            m_Initialized = true;
            return func(instance, &debugMessengerCreateInfo, nullptr, &callback);
        }
        else
        {
            return false;
        }
    }

    VulkanValidation::~VulkanValidation()
    {
        if (!m_Initialized)
        {
            return;
        }
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(m_Instance, callback, nullptr);
        }
    };

} // namespace Cosmo