#pragma once
#include "pch.h"
#include "rhi/RHIEnums.h"

namespace Zephyr
{
    struct BufferDescription
    {
        uint32_t         size;
        BufferUsage      usage;
        BufferMemoryType memoryType;
        ShaderStage      shaderStages;
        PipelineStage    pipelines;
    };

    struct BufferUpdateDescriptor
    {
        uint32_t srcOffset;
        uint32_t dstOffset;

        void*    data;
        uint32_t size;
    };
    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;
    private:

    };
}