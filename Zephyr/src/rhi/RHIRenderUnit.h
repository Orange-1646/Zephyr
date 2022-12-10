#pragma once
#include "Handle.h"
#include "RHIBuffer.h"

namespace Zephyr
{
    struct RenderUnitDescriptor
    {
        Handle<RHIBuffer> vertex;
        Handle<RHIBuffer> index;
        uint32_t          vertexOffset;
        uint32_t          indexOffset;
        uint32_t          indexCount;
    };

    class RHIRenderUnit
    {
    public:
        virtual ~RHIRenderUnit() = default;
    };
}