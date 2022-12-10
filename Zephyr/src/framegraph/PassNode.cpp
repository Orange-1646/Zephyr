#include "PassNode.h"
#include "FrameGraph.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    PassNode::PassNode(FrameGraph* fg, FrameGraphPassBase* pass) : m_FG(fg), m_Pass(pass) {}

    PassNode::~PassNode() { delete m_Pass; }

    void PassNode::Devirtualize(RenderResourceManager* manager)
    {

        if (m_Pass->GetName() == "color pass" || m_Pass->GetName() == "resolve")
        {
            int a = 0;
        }

        for (auto& resource : m_Devirtualize)
        {
            resource->Devirtualize(manager);
        }
        if (m_RTDescriptor.IsValid())
        {
            RenderTargetDescription rtDesc {};
            rtDesc.useDepthStencil = m_RTDescriptor.useDepth;
            rtDesc.present         = m_RTDescriptor.present;
            std::vector<Handle<RHITexture>> attachments;

            for (auto& color : m_RTDescriptor.color)
            {
                auto  vr   = static_cast<VirtualResource*>(m_FG->GetResource(color.handle));
                auto& desc = rtDesc.color.emplace_back();
                desc       = vr->GetAttachmentDescriptor();
                desc.usage = TextureUsageBits::ColorAttachment;

                attachments.push_back(vr->GetRHITexture());
            }
            if (m_RTDescriptor.useDepth)
            {
                auto vr = static_cast<VirtualResource*>(m_FG->GetResource(m_RTDescriptor.depthStencil.handle));
                rtDesc.depthStencil       = vr->GetAttachmentDescriptor();
                rtDesc.depthStencil.usage = TextureUsageBits::DepthStencilAttachment;
                attachments.push_back(vr->GetRHITexture());
            }

            m_RenderTarget.rt = manager->CreateRenderTarget(rtDesc, attachments);
        }
    }
    void PassNode::Destroy(RenderResourceManager* manager)
    {
        if (m_RTDescriptor.IsValid())
        {
            manager->DestroyRenderTarget(m_RenderTarget.rt);
        }
        for (auto& resource : m_Destroy)
        {
            resource->Destroy(manager);
        }
    }
    void PassNode::Execute(FrameGraph* graph)
    {

        // add pipeline barrier
        // we combine all read usages and determin what stage should we wait
        TextureUsage readsUsage = 0;
        for (auto& read : m_Reads)
        {
            auto r = static_cast<VirtualResource*>(read);
            readsUsage |= r->GetUsage();
        }
        if (readsUsage != 0)
        {
            m_FG->GetDriver()->SetupBarrier(readsUsage);
        }
        // execute
        m_Pass->Execute(graph, m_RenderTarget);

        // destroy rendertarget
    }
} // namespace Zephyr