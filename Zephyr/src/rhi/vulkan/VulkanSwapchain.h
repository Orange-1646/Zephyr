#pragma once
#include "VulkanCommon.h"
#include "rhi/Handle.h"

namespace Zephyr
{
    class VulkanContext;
    class VulkanDriver;
    class Window;
    class RHITexture;

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain(Window* window, VulkanDriver* driver);
        void Shutdown();
        ~VulkanSwapchain();

        void PrepareFrame(uint32_t frame);
        bool AcquireImage();

        void               WaitAndPresent();
        VkSemaphore        GetImageAcquireSemaphore() { return m_ImageAquireSemaphore[m_CurrentFrameIndex]; }
        VkSemaphore        GetPresentReadySemaphore() { return m_PresentReadySemaphore[m_CurrentFrameIndex]; }
        VkFence            GetFence() { return m_Fences[m_CurrentFrameIndex]; }
        Handle<RHITexture> GetTexture() { 
            123123;
            
            return m_SwapchainTextures[m_CurrentImageIndex]; }
        inline VkFormat    GetSurfaceFormat() const { return m_SelectedSurfaceFormat.format; }

    private:
        void CreateSurface();
        void CreateSwapchain(uint32_t width, uint32_t height);

        void RecreateSwapchain();

    private:
        VulkanDriver*  m_Driver;
        Window*        m_Window;
        VulkanContext* m_Context;
        VkSurfaceKHR   m_Surface;
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

        uint32_t m_CurrentFrameIndex = 0;
        uint32_t m_CurrentImageIndex = 0;

        std::vector<VkSurfaceFormatKHR> m_SurfaceFormats;
        VkSurfaceFormatKHR              m_SelectedSurfaceFormat;

        uint32_t m_CurrentWidth  = 0;
        uint32_t m_CurrentHeight = 0;

        std::vector<VkFence>     m_Fences;
        std::vector<VkSemaphore> m_ImageAquireSemaphore;
        std::vector<VkSemaphore> m_PresentReadySemaphore;

        uint32_t                        m_ImageCount = 0;
        std::vector<VkImage>            m_SwapchainImages;
        std::vector<Handle<RHITexture>> m_SwapchainTextures;

        bool m_SwapchainStale = false;
    };
} // namespace Zephyr