#include "VulkanSwapchain.h"
#include "VulkanContext.h"
#include "VulkanDriver.h"
#include "core/event/EventSystem.h"
#include "platform/Window.h"

namespace Zephyr
{
    VulkanSwapchain::VulkanSwapchain(Window* window, VulkanDriver* driver) :m_Driver(driver), m_Window(window), m_Context(driver->GetContext())
    {
        CreateSurface();
        RecreateSwapchain();
        // TODO: add window resize listener
    }

    void VulkanSwapchain::Shutdown()
    {
        for (auto& tex : m_SwapchainTextures)
        {
            m_Driver->DestroyTexture(tex);
        }
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        VkDevice   device   = m_Context->m_Device;
        VkInstance instance = m_Context->m_Instance;

        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
        vkDestroySurfaceKHR(instance, m_Surface, nullptr);
        for (auto& fence : m_Fences)
        {
            vkDestroyFence(device, fence, nullptr);
        }
        for (auto& sem : m_ImageAquireSemaphore)
        {
            vkDestroySemaphore(device, sem, nullptr);
        }
        for (auto& sem : m_PresentReadySemaphore)
        {
            vkDestroySemaphore(device, sem, nullptr);
        }
    }

    void VulkanSwapchain::PrepareFrame(uint32_t frame)
    {
        m_CurrentFrameIndex = frame;
        m_SwapchainStale    = false;
    }

    bool VulkanSwapchain::AcquireImage()
    {
        auto device = m_Context->Device();

        vkWaitForFences(device, 1, &m_Fences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

        auto result = vkAcquireNextImageKHR(device,
                              m_Swapchain,
                              10000000000,
                              m_ImageAquireSemaphore[m_CurrentFrameIndex],
                              VK_NULL_HANDLE,
                                            &m_CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapchainStale = true;
            RecreateSwapchain();
            return false;
        }

        vkResetFences(device, 1, &m_Fences[m_CurrentFrameIndex]);
        return true;
    }

    void VulkanSwapchain::WaitAndPresent()
    {

        //if (m_SwapchainStale)
        //{
        //    return;
        //}
        assert(m_PresentReadySemaphore[m_CurrentFrameIndex] == GetPresentReadySemaphore());
        VkPresentInfoKHR presentInfo {};

        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &m_PresentReadySemaphore[m_CurrentFrameIndex];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        VK_CHECK(vkQueuePresentKHR(m_Context->m_GraphicsQueue, &presentInfo), "Swapchain Presentation");
        //auto b     = std::chrono::steady_clock::now();
        //auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count() / 1000000;
        //std::cout << "Present time: " << delta << std::endl;
    }

    void VulkanSwapchain::CreateSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo {};
        createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd      = m_Window->GetNativeWindowHandleWin32();

        VK_CHECK(vkCreateWin32SurfaceKHR(m_Context->m_Instance, &createInfo, nullptr, &m_Surface), "Surface Creation");

        // get and select surface format
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->m_PhysicalDevice, m_Surface, &count, nullptr);

        m_SurfaceFormats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->m_PhysicalDevice, m_Surface, &count, m_SurfaceFormats.data());

        // TODO: we could use a better surface format selection method
        m_SelectedSurfaceFormat = m_SurfaceFormats[0];
    }

    void VulkanSwapchain::CreateSwapchain(uint32_t width, uint32_t height)
    {
        m_CurrentWidth  = width;
        m_CurrentHeight = height;

        VkSwapchainKHR oldSwapchain = m_Swapchain;

        VkSurfaceCapabilitiesKHR cap;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context->m_PhysicalDevice, m_Surface, &cap);

        VkExtent2D extent {};

        if (cap.currentExtent.width == (uint32_t)-1)
        {
            extent.width  = width;
            extent.height = height;
        }
        else
        {
            extent.width  = cap.currentExtent.width;
            extent.height = cap.currentExtent.height;
        }

        uint32_t desiredImageCount = cap.minImageCount + 1;
        if (desiredImageCount > cap.maxImageCount && cap.maxImageCount > 0)
        {
            desiredImageCount = cap.maxImageCount;
        }

        uint32_t graphicsQueueIndex = m_Context->m_QueueFamilyIndices.graphics;

        // use current transform
        auto transform = cap.currentTransform;
        // find available composite alpha
        auto compositeAlpha          = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        auto supportedCompositeAlpha = cap.supportedCompositeAlpha;

        std::vector<VkCompositeAlphaFlagBitsKHR> alphaFlags = {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                               VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                                               VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                                                               VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

        for (auto& alpha : alphaFlags)
        {
            if ((supportedCompositeAlpha & alpha) != 0)
            {
                compositeAlpha = alpha;
                break;
            }
        }

        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.oldSwapchain     = m_Swapchain;
        createInfo.surface          = m_Surface;
        createInfo.minImageCount    = desiredImageCount;
        createInfo.imageFormat      = m_SelectedSurfaceFormat.format;
        createInfo.imageColorSpace  = m_SelectedSurfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        // the plan is to use the last graphics pass to do bloom composite + tonemapping + color grading + gamma
        // correction so we can get away with using VK_SHARING_MODE_EXCLUSIVE if we want to write directly to it via
        // compute shader/ copy to it in compute command buffer, we need to use VK_SHARING_MODE_CONCURRENT. but we
        // really dont :)
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 1;
        createInfo.pQueueFamilyIndices   = &graphicsQueueIndex;
        createInfo.preTransform          = transform;
        createInfo.compositeAlpha        = compositeAlpha;
        createInfo.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
        createInfo.clipped               = VK_FALSE;
        createInfo.oldSwapchain          = oldSwapchain;

        VK_CHECK(vkCreateSwapchainKHR(m_Context->m_Device, &createInfo, nullptr, &m_Swapchain), "Swapchain Creation");

        // destroy old resources
        if (oldSwapchain)
        {
            vkDestroySwapchainKHR(m_Context->m_Device, oldSwapchain, nullptr);
        }
        // destroy old image views

        // retreive swapchain images
        vkGetSwapchainImagesKHR(m_Context->m_Device, m_Swapchain, &m_ImageCount, nullptr);
        m_SwapchainImages.resize(m_ImageCount);
        vkGetSwapchainImagesKHR(m_Context->m_Device, m_Swapchain, &m_ImageCount, m_SwapchainImages.data());
        
        for (auto& texture : m_SwapchainTextures)
        {
            m_Driver->DestroyTexture(texture);
        }

        m_SwapchainTextures.resize(m_ImageCount);

        uint32_t i = 0;
        for (auto& texture : m_SwapchainTextures)
        {
            TextureDescription desc {};
            desc.width = extent.width;
            desc.height = extent.height;
            desc.depth  = 1;
            desc.format = TextureFormat::DEFAULT;
            desc.levels = 1;
            desc.pipelines = PipelineTypeBits::Graphics;
            desc.sampler   = SamplerType::Sampler2D;
            desc.samples   = 1;
            desc.usage     = TextureUsageBits::ColorAttachment;

            texture = m_Driver->CreateTexture(desc, m_SwapchainImages[i], m_SelectedSurfaceFormat.format);
            i++;
        }

        // create sync object if necessary
        if (m_Fences.empty())
        {
            m_Fences.resize(MAX_CONCURRENT_FRAME);
            m_PresentReadySemaphore.resize(MAX_CONCURRENT_FRAME);
            m_ImageAquireSemaphore.resize(MAX_CONCURRENT_FRAME);

            for (uint32_t i = 0; i < MAX_CONCURRENT_FRAME; i++)
            {
                {
                    VkFenceCreateInfo createInfo {};
                    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                    VK_CHECK(vkCreateFence(m_Context->m_Device, &createInfo, nullptr, &m_Fences[i]), "Fence Creation");
                }
                {
                    VkSemaphoreCreateInfo createInfo {};
                    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                    VK_CHECK(vkCreateSemaphore(m_Context->m_Device, &createInfo, nullptr, &m_ImageAquireSemaphore[i]),
                             "Sempahore Creation");
                    VK_CHECK(vkCreateSemaphore(m_Context->m_Device, &createInfo, nullptr, &m_PresentReadySemaphore[i]),
                             "Sempahore Creation");
                }
            }
        }
    }

    void VulkanSwapchain::RecreateSwapchain()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context->m_PhysicalDevice, m_Surface, &surfaceCapabilities);

        uint32_t newWidth  = surfaceCapabilities.currentExtent.width;
        uint32_t newHeight = surfaceCapabilities.currentExtent.height;

        while (newWidth == 0 || newHeight == 0)
        {
            m_Window->GetFramebufferSize(&newWidth, &newHeight);
            m_Window->WaitEvents();
        }

        if (newWidth == m_CurrentWidth && newHeight == m_CurrentHeight)
        {
            return;
        }

        m_CurrentWidth  = newWidth;
        m_CurrentHeight = newHeight;

        vkDeviceWaitIdle(m_Context->m_Device);
        CreateSwapchain(newWidth, newHeight);
    }
} // namespace Zephyr