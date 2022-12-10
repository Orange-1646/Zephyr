#include "VulkanRenderTarget.h"
#include "VulkanDriver.h"
#include "VulkanTexture.h"
#include "VulkanUtil.h"

namespace Zephyr
{
    VulkanRenderTarget::VulkanRenderTarget(VulkanDriver*                          driver,
                                           const RenderTargetDescription&         desc,
                                           const std::vector<Handle<RHITexture>>& attachments) :
        m_Descriptor(desc)
    {
        auto& context = *driver->GetContext();

        // create render pass
        // note that we dont need subpass dependencies. we'll handle layout transition explicitly with
        // image memory barrier
        {
            std::vector<VkAttachmentDescription> attachmentDesc;
            uint32_t                             i = 0;
            auto format                            = driver->GetResource<VulkanTexture>(attachments[i])->GetFormat();
            for (auto& color : desc.color)
            {
                auto  c           = driver->GetResource<VulkanTexture>(attachments[i]);
                m_Colors.push_back(c);
                auto& cd = attachmentDesc.emplace_back();
                cd.format = format == TextureFormat::DEFAULT ?
                    driver->GetSurfaceFormat() : 
                    VulkanUtil::GetTextureFormat(c->GetFormat());
                cd.samples        = VK_SAMPLE_COUNT_1_BIT;
                cd.loadOp         = color.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                cd.storeOp        = color.save ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
                cd.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                cd.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                cd.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                // We assume that if the image is not used for final presentation, it should be used as a sampler input in another pipeline
                // or this attachment wouldn't make sense
                cd.finalLayout    = desc.present ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                i++;
            }
            if (desc.useDepthStencil)
            {
                m_DepthStencil = driver->GetResource<VulkanTexture>(attachments[i]);
                auto& dsd = attachmentDesc.emplace_back();
                dsd.format = VulkanUtil::GetTextureFormat(m_DepthStencil->GetFormat());
                dsd.samples = VK_SAMPLE_COUNT_1_BIT;
                dsd.loadOp  = desc.depthStencil.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                dsd.storeOp = desc.depthStencil.save ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
                dsd.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                dsd.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                dsd.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                dsd.finalLayout    = desc.depthStencil.save ?
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                i++;
            }

            std::vector<VkAttachmentReference> cRef {};

            for (uint32_t i = 0; i < desc.color.size(); i++)
            {
                auto& r      = cRef.emplace_back();
                r.attachment = i;
                r.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            VkAttachmentReference dsRef {};
            dsRef.attachment = desc.color.size();
            dsRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass {};
            subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = 0;
            subpass.pInputAttachments    = nullptr;
            subpass.colorAttachmentCount = cRef.size();
            subpass.pColorAttachments    = cRef.data();
            if (desc.useDepthStencil)
            {
                subpass.pDepthStencilAttachment = &dsRef;
            }

            VkRenderPassCreateInfo createInfo {};
            createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            createInfo.attachmentCount = attachmentDesc.size();
            createInfo.pAttachments    = attachmentDesc.data();
            createInfo.subpassCount    = 1;
            createInfo.pSubpasses      = &subpass;

            VK_CHECK(vkCreateRenderPass(context.Device(), &createInfo, nullptr, &m_RenderPass), "Render Pass Creation");
        }
        // create framebuffer
        {
            std::vector<VkImageView> views;
            uint32_t                 width;
            uint32_t                 height;

            uint32_t i = 0;
            for (auto& color : desc.color)
            {
                ViewRange range {};
                range.baseLayer  = color.layer;
                range.layerCount = 1;
                range.baseLevel  = color.level;
                range.levelCount = 1;

                auto image     = driver->GetResource<VulkanTexture>(attachments[i]);
                auto dimension = image->GetDimension();

                m_Width  = dimension.first;
                m_Height = dimension.second;

                auto view = image->GetView(driver, range);
                views.push_back(view);
                width  = dimension.first;
                height = dimension.second;

                i++;
            }

            if (desc.useDepthStencil)
            {
                ViewRange range {};
                range.baseLayer  = desc.depthStencil.layer;
                range.layerCount = 1;
                range.baseLevel  = desc.depthStencil.level;
                range.levelCount = 1;

                auto image     = driver->GetResource<VulkanTexture>(attachments[i]);
                auto dimension = image->GetDimension();

                m_Width  = dimension.first;
                m_Height = dimension.second;

                auto view = driver->GetResource<VulkanTexture>(attachments[i])->GetView(driver, range);
                views.push_back(view);

                i++;
            }

            VkFramebufferCreateInfo createInfo {};
            createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass      = m_RenderPass;
            createInfo.attachmentCount = views.size();
            createInfo.pAttachments    = views.data();
            createInfo.width           = m_Width;
            createInfo.height          = m_Height;
            createInfo.layers          = 1;

            VK_CHECK(vkCreateFramebuffer(context.Device(), &createInfo, nullptr, &m_Framebuffer),
                     "Framebuffer Creation");
        }
    }

    void VulkanRenderTarget::Destroy(VulkanDriver* driver)
    {
        // destroy framebuffer
        vkDestroyFramebuffer(driver->GetContext()->Device(), m_Framebuffer, nullptr);
        vkDestroyRenderPass(driver->GetContext()->Device(), m_RenderPass, nullptr);
    }
    void VulkanRenderTarget::Begin(VkCommandBuffer cb)
    {
        std::vector<VkClearValue> clearValues;

        for (auto& color : m_Descriptor.color)
        {
            auto& value = clearValues.emplace_back();
            value.color = {.2, .3, .4, 1.};
        }

        if (m_Descriptor.useDepthStencil)
        {
            auto& value                = clearValues.emplace_back();
            value.depthStencil.depth   = 1.;
            value.depthStencil.stencil = 1;
        }

        VkRenderPassBeginInfo beginInfo {};
        beginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass               = m_RenderPass;
        beginInfo.renderArea.offset.x      = 0;
        beginInfo.renderArea.offset.y      = 0;
        beginInfo.renderArea.extent.width  = m_Width;
        beginInfo.renderArea.extent.height = m_Height;
        beginInfo.clearValueCount          = clearValues.size();
        beginInfo.pClearValues             = clearValues.data();
        beginInfo.framebuffer              = m_Framebuffer;

        vkCmdBeginRenderPass(cb, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    void VulkanRenderTarget::End(VkCommandBuffer cb) { 

        for (uint32_t i = 0; i < m_Descriptor.color.size(); i++)
        {
            auto& color = m_Colors[i];
            auto& descriptor = m_Descriptor.color[i];
            color->SetLayout(0, descriptor.layer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        if (m_Descriptor.useDepthStencil)
        {
            auto& descriptor = m_Descriptor.depthStencil;
            m_DepthStencil->SetLayout(0, descriptor.layer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        vkCmdEndRenderPass(cb); }
} // namespace Zephyr