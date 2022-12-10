#pragma once
#include "Blackboard.h"
#include "DAG.h"
#include "FrameGraphResource.h"
#include "PassNode.h"
#include "ResourceNode.h"
#include "VirtualResource.h"
#include "pch.h"

namespace Zephyr
{
    class Engine;
    class Driver;

    struct ResourceSlot
    {
        uint32_t virtualResourceIndex = 0;
        uint32_t resourceNodeIndex = 0;
    };

    class FrameGraph
    {
    public:
        FrameGraph(Engine* engine, RenderResourceManager* manager) : m_Engine(engine), m_Manager(manager) {}
        ~FrameGraph() {

            for (auto& passNode: m_PassNodes)
            {
                delete passNode;
            }

            for (auto& resourceNode : m_ResourceNodes)
            {
                delete resourceNode;
            }

            for (auto& virtualResource : m_VirtualResources)
            {
                delete virtualResource;
            }

            for (auto& edge : m_Edges)
            {
                delete edge;
            }
        }

        inline Blackboard& GetBlackboard() { return m_Blackboard; }

        template<typename Data, typename Setup, typename Execute>
        FrameGraphPass<Data, Execute>* AddPass(const std::string& passName, Setup&& setup, Execute&& execute)
        {
            auto pass     = new FrameGraphPass<Data, Execute>(passName, std::forward<Execute>(execute));
            auto passNode = CreatePassNode(pass);

            setup(this, passNode, pass->GetData());

            return pass;
        }

        void Compile();

        void Execute();

        // create a texture handle. this would also create a virtual resource and a resource node
        FrameGraphResourceHandle<FrameGraphTexture> CreateTexture(const TextureDescription& desc, bool external = false);

        // this creates a subresource from the "main" resource. subresource represents a view into the actual resource.
        // for example, for cascaded shadow map, the shadowmap texture would be the "main" resource, and each layer of
        // which each cascade renders into would be a subresource. a write/ read from the subresource is effectively a write/ read
        // from the "main" resource
        FrameGraphResourceHandle<FrameGraphTexture>
        CreateSubresource(FrameGraphResourceHandle<FrameGraphTexture> parent,
                          const FrameGraphTexture::SubresourceDescriptor& desc);

        void Write(PassNode* node, FrameGraphResourceHandle<FrameGraphTexture> target);
        void Read(PassNode* node, FrameGraphResourceHandle<FrameGraphTexture> target);
        void SetRenderTarget(PassNode* node, const FrameGraphRenderTargetDescriptor& target);

        Driver* GetDriver();
    public:
        ResourceNode* GetNode(const FrameGraphResourceHandle<FrameGraphTexture>& resource);
        VirtualResourceBase* GetResource(const FrameGraphResourceHandle<FrameGraphTexture>& resource);

        ResourceNode* CreateResourceNode(uint32_t id);
        PassNode*     CreatePassNode(FrameGraphPassBase* pass);
        Edge*         CreateEdge(Node* to, Node* from);

    private:
        Engine* m_Engine;
        RenderResourceManager* m_Manager;

        DAG m_Graph;

        std::vector<ResourceSlot>                                  m_Slots;
        std::vector<PassNode*>                                     m_PassNodes;
        std::vector<ResourceNode*>                                 m_ResourceNodes;
        std::vector<VirtualResourceBase*>                          m_VirtualResources;
        std::vector<Edge*>                                         m_Edges;
        Blackboard                                                 m_Blackboard;

        std::vector<PassNode*> m_ActivePassNodes;

        friend PassNode;
    };

} // namespace Zephyr