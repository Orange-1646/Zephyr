#include "FrameGraph.h"
#include "engine/Engine.h"

namespace Zephyr
{
    void FrameGraph::Compile()
    {
        m_Graph.Cull();

        // add resource dependencies

        std::copy_if(m_PassNodes.begin(), m_PassNodes.end(), std::back_inserter(m_ActivePassNodes), [](PassNode* node) {
            return !node->IsCulled();
        });

        for (auto& node : m_ActivePassNodes)
        {
            auto& incomingEdges = m_Graph.GetIncomingEdges(node);

            for (auto& edge : incomingEdges)
            {
                // TODO: add type safety check
                auto resourceNode = static_cast<ResourceNode*>(edge->from);
                auto resource     = GetResource(resourceNode->GetHandle());
                resource->AddPassDependency(node);
            }

            auto& outgoingEdges = m_Graph.GetOutgoingEdges(node);

            for (auto& edge : outgoingEdges)
            {
                if (node->GetName() == "color pass")
                {
                    std::vector<uint32_t> a;
                    123123;
                }
                // TODO: add type safety check
                auto  resourceNode = static_cast<ResourceNode*>(edge->to);
                auto& resource1    = resourceNode->GetHandle();
                auto  resource     = GetResource(resourceNode->GetHandle());
                resource->AddPassDependency(node);
            }
        }

        for (auto& resource : m_VirtualResources)
        {
            if (resource->IsSubresource() || resource->IsCulled())
            {
                continue;
            }
            auto first = resource->First();
            auto last  = resource->Last();

            first->AddDevirtualize(resource);
            last->AddDestroy(resource);
        }
    }

    void FrameGraph::Execute()
    {
        for (auto& node : m_ActivePassNodes)
        {
            // devirtualize
            node->Devirtualize(m_Manager);

            // execute
            node->Execute(this);
            // destroy
            node->Destroy(m_Manager);
        }
    }

    FrameGraphResourceHandle<FrameGraphTexture> FrameGraph::CreateTexture(const TextureDescription& desc, bool external)
    {
        // create root virtual resource
        uint32_t resourceId = m_VirtualResources.size();
        uint32_t nodeId     = m_ResourceNodes.size();

        uint32_t id = m_Slots.size();

        auto vr           = new VirtualResource(desc, external);
        auto resourceNode = CreateResourceNode(id);
        m_VirtualResources.push_back(vr);
        m_ResourceNodes.push_back(resourceNode);
        m_Slots.push_back({resourceId, nodeId});

        return FrameGraphResourceHandle<FrameGraphTexture>(id);
    }

    FrameGraphResourceHandle<FrameGraphTexture>
    FrameGraph::CreateSubresource(FrameGraphResourceHandle<FrameGraphTexture>     parent,
                                  const FrameGraphTexture::SubresourceDescriptor& desc)
    {
        // this would create a virtual resource, but resource node would be the same as parent resource node
        uint32_t resourceId = m_VirtualResources.size();
        uint32_t nodeId     = m_Slots[parent.GetID()].resourceNodeIndex;
        uint32_t id         = m_Slots.size();

        auto vr = new VirtualResource(GetResource(parent), desc);
        m_VirtualResources.push_back(vr);
        m_Slots.push_back({resourceId, nodeId});

        return FrameGraphResourceHandle<FrameGraphTexture>(id);
    }

    // declare a write from renderpass to resource
    void FrameGraph::Write(PassNode* node, FrameGraphResourceHandle<FrameGraphTexture> target, TextureUsage usage)
    {
        auto resourceNode = GetNode(target);
        CreateEdge(resourceNode, node);
        node->AddWrite(GetResource(target), usage);
    }

    // declare a read from resource to renderpass
    void FrameGraph::Read(PassNode* node, FrameGraphResourceHandle<FrameGraphTexture> target, TextureUsage usage)
    {

        auto resourceNode = GetNode(target);
        CreateEdge(node, resourceNode);
        node->AddRead(GetResource(target), usage);
    }

    void FrameGraph::SetRenderTarget(PassNode* node, const FrameGraphRenderTargetDescriptor& target)
    {
        node->SetRenderTarget(target);
    }

    Driver* FrameGraph::GetDriver() { return m_Engine->GetDriver(); }

    ResourceNode* FrameGraph::GetNode(const FrameGraphResourceHandle<FrameGraphTexture>& resource)
    {
        auto handleID = resource.GetID();
        assert(handleID < m_Slots.size());
        auto nodeID = m_Slots[resource.GetID()].resourceNodeIndex;
        assert(nodeID < m_ResourceNodes.size());

        return m_ResourceNodes[nodeID];
    }

    VirtualResourceBase* FrameGraph::GetResource(const FrameGraphResourceHandle<FrameGraphTexture>& resource)
    {
        auto handleID = resource.GetID();
        assert(handleID < m_Slots.size());
        auto resourceID = m_Slots[resource.GetID()].virtualResourceIndex;
        assert(resourceID < m_VirtualResources.size());

        return m_VirtualResources[resourceID];
    }

    ResourceNode* FrameGraph::CreateResourceNode(uint32_t id)
    {
        auto node = new ResourceNode(id);
        m_Graph.Add(node);
        return node;
    }

    PassNode* FrameGraph::CreatePassNode(FrameGraphPassBase* pass)
    {
        auto node = new PassNode(this, pass);
        m_Graph.Add(node);
        m_PassNodes.push_back(node);
        return node;
    }

    // TODO: could use some type safety check
    Edge* FrameGraph::CreateEdge(Node* to, Node* from)
    {
        auto edge = new Edge(to, from);
        m_Graph.Add(edge);
        m_Edges.push_back(edge);
        return edge;
    }
} // namespace Zephyr