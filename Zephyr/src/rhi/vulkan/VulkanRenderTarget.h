#pragma once
#include "VulkanCommon.h"
#include "rhi/RHIRenderTarget.h"

namespace Zephyr
{
    class VulkanDriver;
    class VulkanTexture;
    class VulkanRenderTarget : public RHIRenderTarget
    {
    public:
        VulkanRenderTarget(VulkanDriver*                          driver,
                           const RenderTargetDescription&         desc,
                           const std::vector<Handle<RHITexture>>& attachments);
        ~VulkanRenderTarget() override = default;
        void                                  Destroy(VulkanDriver* driver);
        void                                  Begin(VkCommandBuffer cb);
        void                                  End(VkCommandBuffer cb);
        inline VkRenderPass                   GetRenderPass() { return m_RenderPass; }
        inline VkFramebuffer                  GetFramebuffer() { return m_Framebuffer; }
        inline const RenderTargetDescription& GetDescriptor() { return m_Descriptor; }

    private:
        RenderTargetDescription m_Descriptor;
        VkRenderPass            m_RenderPass;
        VkFramebuffer           m_Framebuffer;

        std::vector<VulkanTexture*> m_Colors;
        VulkanTexture*             m_DepthStencil = nullptr;

        uint32_t m_Width;
        uint32_t m_Height;

        // whether we're rendering directly into the swapchain
        bool m_External = false;
    };
} // namespace Zephyr