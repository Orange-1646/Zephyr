#include "VulkanContext.h"
#include "VulkanValidation.h"
#include "core/log/Logger.h"

namespace Zephyr
{
    VulkanContext::VulkanContext() { 
        CreateInstance();
        PickPhysicalDevice();
        GetDepthFormat();
        GetQueueFamilyIndices();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateDescriptorPool();
        CreateTimeQuery();
    }

    VulkanContext::~VulkanContext() { Cleanup(); }

    void VulkanContext::Cleanup() {
        vkDeviceWaitIdle(m_Device);
        vkDestroyDescriptorPool(m_Device, m_GlobalDescriptorPool, nullptr);
        vkDestroyCommandPool(m_Device, m_GlobalGraphicsCommandPool, nullptr);
        vkDestroyCommandPool(m_Device, m_GlobalComputeCommandPool, nullptr);
        vkDestroyDevice(m_Device, nullptr);
        delete m_Validation;
        vkDestroyInstance(m_Instance, nullptr);
    }

    VkCommandBuffer VulkanContext::BeginSingleTimeCommandBuffer() {
        VkCommandBuffer commandBuffer;

        VkCommandBufferAllocateInfo allocateInfo {};
        allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.commandPool        = m_GlobalGraphicsCommandPool;
        allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocateInfo, &commandBuffer),
                 "One Time Command Buffer Allocation");

        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo), "One Time Command Buffer Begin");

        return commandBuffer;
    }

    void VulkanContext::EndSingleTimeCommandBuffer(VkCommandBuffer cb) {

        VK_CHECK(vkEndCommandBuffer(cb), "One Time Command Buffer End");

        VkSubmitInfo submitInfo {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &cb;

        // create a fence to wait on
        // TODO: this might not be optimal
        VkFenceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = 0;

        VkFence fence;
        VK_CHECK(vkCreateFence(m_Device, &createInfo, nullptr, &fence), "CB Fence Creation");

        VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence), "One Time Command Buffer Submission");

        VK_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX), "Failed at waiting for cb fence");

        vkDestroyFence(m_Device, fence, nullptr);

        vkFreeCommandBuffers(m_Device, m_GlobalGraphicsCommandPool, 1, &cb);
    }

    void VulkanContext::CreateInstance() { 
        m_Validation = new VulkanValidation();

        VkApplicationInfo appInfo {};
        appInfo.apiVersion = VK_API_VERSION_1_3;

        // enable validation layer
        std::vector<const char*> instanceLayers;
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");

        // 0: surface extension for surface and swapchain
        // 1: win32 system surface platform extension
        // 3: debug utils
        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

        VkInstanceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = instanceLayers.size();
        createInfo.ppEnabledLayerNames = instanceLayers.data();
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.pNext                   = &m_Validation->debugMessengerCreateInfo;

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance), "Instance Creation");

        m_Validation->Init(m_Instance);
    }

    void VulkanContext::PickPhysicalDevice() {
        // first find all usable GPU
        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        assert(deviceCount > 0);

        std::vector<VkPhysicalDevice> deviceList(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, deviceList.data());

        // find suitable gpu
        for (auto& device : deviceList)
        {
            VkPhysicalDeviceProperties prop;
            vkGetPhysicalDeviceProperties(device, &prop);

            if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                printf("Found Discrete GPU: %s\n", prop.deviceName);
                m_PhysicalDevice = device;
                break;
            }
        }
        assert(m_PhysicalDevice);

        // get queue family properties(for later queue creation)
        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, nullptr);

        m_QueueFamilyProperties.resize(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, m_QueueFamilyProperties.data());

        // get physical device memery properties(for buffer && image memory allocation)
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalDeviceMemoryProperties);

        // get supported device extensions
        uint32_t extCount;
        
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
        m_SupportedExtensions.resize(extCount);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, m_SupportedExtensions.data());
        
    }

    void VulkanContext::GetDepthFormat() {

        const VkFormat desiredFormats[] = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

        bool foundDepthFormat = false;

        for (uint32_t i = 0; i < sizeof(desiredFormats) / sizeof(desiredFormats[0]); i++)
        {
            VkFormatProperties prop;

            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, desiredFormats[i], &prop);

            if (prop.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                foundDepthFormat = true;
                m_DepthFormat    = desiredFormats[i];
                break;
            }
        }

        assert(foundDepthFormat);
    }

    void VulkanContext::GetQueueFamilyIndices() {

        // first try to find dedicated compute and transfer queue

        for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
        {
            auto& props = m_QueueFamilyProperties[i];
            if ((props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            {
                m_QueueFamilyIndices.compute = i;
                continue;
            }

            if ((props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
                (props.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
            {
                m_QueueFamilyIndices.transfer = i;
                continue;
            }
        }

        // for the rest, simply pick the foremost index
        for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
        {
            auto& props = m_QueueFamilyProperties[i];
            if (m_QueueFamilyIndices.graphics == -1 && props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_QueueFamilyIndices.graphics = i;
            }
            if (m_QueueFamilyIndices.compute == -1 && props.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                m_QueueFamilyIndices.compute = i;
            }
            if (m_QueueFamilyIndices.transfer == -1 && props.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                m_QueueFamilyIndices.transfer = i;
            }
        }

        assert(m_QueueFamilyIndices.graphics > -1 && m_QueueFamilyIndices.compute > -1 &&
               m_QueueFamilyIndices.transfer > -1);
    }

    void VulkanContext::CreateLogicalDevice() { 
        // queue info
        float priority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfo(3);
        queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[0].queueCount = 1;
        queueCreateInfo[0].pQueuePriorities = &priority;
        queueCreateInfo[0].queueFamilyIndex = m_QueueFamilyIndices.graphics;

        queueCreateInfo[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[1].queueCount       = 1;
        queueCreateInfo[1].pQueuePriorities = &priority;
        queueCreateInfo[1].queueFamilyIndex = m_QueueFamilyIndices.compute;

        queueCreateInfo[2].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[2].queueCount       = 1;
        queueCreateInfo[2].pQueuePriorities = &priority;
        queueCreateInfo[2].queueFamilyIndex = m_QueueFamilyIndices.transfer;

        // layers
        std::vector<const char*> layers {};

        // extensions
        std::vector<const char*> extensions {"VK_KHR_swapchain"};

        for (auto& extension : extensions)
        {
            m_SupportedExtensions;
            assert(IsDeviceExtensionSupported(extension));
        }

        VkDeviceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = queueCreateInfo.size();
        createInfo.pQueueCreateInfos = queueCreateInfo.data();
        createInfo.enabledLayerCount    = layers.size();
        createInfo.ppEnabledLayerNames  = layers.data();
        createInfo.enabledExtensionCount   = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        VK_CHECK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "Device Creation");

        // get all queue handles
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.graphics, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.compute, 0, &m_ComputeQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.transfer, 0, &m_TransferQueue);
    }

    void VulkanContext::CreateCommandPool() { 
        VkCommandPoolCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = m_QueueFamilyIndices.graphics;
        createInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK(vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_GlobalGraphicsCommandPool),
                 "Global Graphics Command Pool Creation");

        createInfo.queueFamilyIndex = m_QueueFamilyIndices.compute;

        VK_CHECK(vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_GlobalComputeCommandPool),
                 "Global Compute Command Pool Creation");
    }

    void VulkanContext::CreateDescriptorPool() {
        VkDescriptorPoolSize       poolSize[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
        VkDescriptorPoolCreateInfo createInfo   = {};
        createInfo.sType                        = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags                        = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        createInfo.maxSets                      = 100000;
        createInfo.poolSizeCount                = (uint32_t)sizeof(poolSize) / sizeof(poolSize[0]);
        createInfo.pPoolSizes                   = poolSize;

        VK_CHECK(vkCreateDescriptorPool(m_Device, &createInfo, nullptr, &m_GlobalDescriptorPool), "Descriptor Pool Creation");
    }

    void VulkanContext::CreateTimeQuery() {
        // TODO: implement
    }

    bool VulkanContext::IsDeviceExtensionSupported(const char* extension)
    {
        for (auto& prop : m_SupportedExtensions)
        {
            if (strcmp(prop.extensionName, extension) == 0)
            {
                return true;
            }
        }
        return false;
    }
} // namespace Zephyr