#pragma once
#include "DAG.h"
#include "FrameGraphResource.h"
#include "VirtualResource.h"

namespace Zephyr
{
    class PassNode;
    class ResourceNode : public Node
    {
    public:
        inline FrameGraphResourceHandle<FrameGraphTexture> GetHandle() { return handle; }

        FrameGraphResourceHandle<FrameGraphTexture> handle;

        ResourceNode(const FrameGraphResourceHandle<FrameGraphTexture>& handle) : handle(handle) {}
    };
} // namespace Zephyr