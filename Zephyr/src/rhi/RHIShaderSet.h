#pragma once
#include "RHIEnums.h"

namespace Zephyr
{
    struct ShaderSetDescription
    {
        VertexType            vertexType;
        PipelineType          pipeline;
        std::vector<uint32_t> vertex;
        std::vector<uint32_t> fragment;
        std::vector<uint32_t> compute;
    };

    class RHIShaderSet
    {
    public:
        virtual ~RHIShaderSet() = default;
    };
} // namespace Zephyr