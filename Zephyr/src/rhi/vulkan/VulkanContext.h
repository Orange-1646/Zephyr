#pragma once
#include "pch.h"
#include "VulkanCommon.h"

namespace Zephyr
{
    class VulkanValidation;

    struct QueueFamilyIndices
    {
        int graphics = -1;
        int compute  = -1;
        int transfer = -1;
    };
    class VulkanContext final
    {
    public:
        VulkanContext();
        ~VulkanContext();

        inline QueueFamilyIndices QueueIndices() const { return m_QueueFamilyIndices; }
        inline VkDevice           Device() const { return m_Device; }
        inline VkPhysicalDevice   PhysicalDevice() const { return m_PhysicalDevice; }
        inline VkDescriptorPool   GetDescriptorPool() const { return m_GlobalDescriptorPool; }
    private:
        void CreateInstance();
        void PickPhysicalDevice();
        void GetDepthFormat();
        void GetQueueFamilyIndices();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateDescriptorPool();
        void CreateTimeQuery();
        void Cleanup();

        VkCommandBuffer BeginSingleTimeCommandBuffer();
        void            EndSingleTimeCommandBuffer(VkCommandBuffer cb);


        bool IsDeviceExtensionSupported(const char* extension);
    private:
        VulkanValidation* m_Validation;
        VkInstance m_Instance;
        VkPhysicalDevice m_PhysicalDevice;
        std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
        VkPhysicalDeviceMemoryProperties     m_PhysicalDeviceMemoryProperties;
        std::vector<VkExtensionProperties>   m_SupportedExtensions;
        VkFormat                             m_DepthFormat;
        QueueFamilyIndices                   m_QueueFamilyIndices;
        VkDevice                             m_Device;
        VkQueue                              m_GraphicsQueue;
        VkQueue                              m_ComputeQueue;
        VkQueue                              m_TransferQueue;
        VkCommandPool m_GlobalGraphicsCommandPool;
        VkCommandPool m_GlobalComputeCommandPool;
        VkDescriptorPool m_GlobalDescriptorPool;

        friend class VulkanSwapchain;
        friend class VulkanDriver;
    };
}