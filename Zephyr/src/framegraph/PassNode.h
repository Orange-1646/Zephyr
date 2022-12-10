#pragma once
#include "DAG.h"
#include "FrameGraphPass.h"
#include "VirtualResource.h"

namespace Zephyr
{
    class RenderResourceManager;
    class FrameGraph;

    struct FrameGraphAttachmentDescriptor
    {
        FrameGraphResourceHandle<FrameGraphTexture> handle;
        bool                                        clear = true;
        bool                                        save  = true;
    };

    struct FrameGraphRenderTargetDescriptor
    {
        std::vector<FrameGraphAttachmentDescriptor> color;
        FrameGraphAttachmentDescriptor              depthStencil;
        bool                                        useDepth = false;
        bool                                        present  = false;

        bool IsValid() { return color.size() > 0 || useDepth; }
    };

    class PassNode : public Node
    {
    public:
        PassNode(FrameGraph* fg, FrameGraphPassBase* pass);
        ~PassNode() override;

        std::string GetName() { return m_Pass->GetName(); }

        void SetRenderTarget(const FrameGraphRenderTargetDescriptor& desc) { m_RTDescriptor = desc; }

        void AddRead(VirtualResourceBase* resource) { m_Reads.push_back(resource); }

        void AddDevirtualize(VirtualResourceBase* resource) { m_Devirtualize.push_back(resource); }

        void AddDestroy(VirtualResourceBase* resource) { m_Destroy.push_back(resource); }

        void Devirtualize(RenderResourceManager* manager);

        void Destroy(RenderResourceManager* manager);

        void Execute(FrameGraph* graph);
        void Log()
        {
            std::cout << "num of resource to devirtualize: " << m_Devirtualize.size() << std::endl;
            std::cout << "num of resource to destroy: " << m_Destroy.size() << std::endl;
        }

    private:
        FrameGraph*         m_FG;
        FrameGraphPassBase* m_Pass;

        FrameGraphRenderTargetDescriptor  m_RTDescriptor;
        std::vector<VirtualResourceBase*> m_Devirtualize;
        std::vector<VirtualResourceBase*> m_Destroy;

        // this is for figuring out what kind of memory barriers we need before executing the pass
        // if we read from a texture that was previously used as depth attachment, we need to wait on
        // late depth test write, for color attachment we need to wait on color attachment output write
        std::vector<VirtualResourceBase*> m_Reads;

        PassRenderTarget m_RenderTarget;
    };
} // namespace Zephyr