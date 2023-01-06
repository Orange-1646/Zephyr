#include "PassNode.h"
#include "FrameGraph.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    PassNode::PassNode(FrameGraph* fg, FrameGraphPassBase* pass) : m_FG(fg), m_Pass(pass) {}

    PassNode::~PassNode() { delete m_Pass; }

    void PassNode::AddRead(VirtualResourceBase* resource, TextureUsage usage)
    {
        auto& read    = m_Reads.emplace_back();
        read.usage    = usage;
        read.resource = resource;
    }

    void PassNode::AddWrite(VirtualResourceBase* resource, TextureUsage usage)
    {

        auto& write    = m_Writes.emplace_back();
        write.usage    = usage;
        write.resource = resource;
    }

    void PassNode::Devirtualize(RenderResourceManager* manager)
    {
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
                rtDesc.depthStencil.clear = m_RTDescriptor.depthStencil.clear;
                rtDesc.depthStencil.save  = m_RTDescriptor.depthStencil.save;
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
        // the assumption here is that if the user didn't setup a proper render target,
        // this pass is a compute pass

        PipelineType type = m_RenderTarget.rt.IsValid() ? PipelineTypeBits::Graphics : PipelineTypeBits::Compute;
        // add pipeline barrier
        for (auto& read : m_Reads)
        {
            auto r = static_cast<VirtualResource*>(read.resource);
            m_FG->GetDriver()->SetupBarrier(r->GetRHITexture(), r->GetViewRange(), read.usage, type);
        }
        for (auto& write : m_Writes)
        {
            auto r = static_cast<VirtualResource*>(write.resource);
            m_FG->GetDriver()->SetupBarrier(r->GetRHITexture(), r->GetViewRange(), write.usage, type);
        }
        // execute
        m_Pass->Execute(graph, m_RenderTarget);

        // destroy rendertarget
    }
} // namespace Zephyr