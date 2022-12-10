#pragma once
#include "DAG.h"
#include "VirtualResource.h"
#include "FrameGraphResource.h"

namespace Zephyr
{
    class PassNode;
    class ResourceNode : public Node
    {
    public:
        [[nodiscard]] inline FrameGraphResourceHandle<FrameGraphTexture> GetHandle() { return handle; }
        
        FrameGraphResourceHandle<FrameGraphTexture> handle;

        ResourceNode(const FrameGraphResourceHandle<FrameGraphTexture>& handle) : handle(handle) {}
    };
}