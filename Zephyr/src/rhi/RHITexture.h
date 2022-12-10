#pragma once
#include "RHIEnums.h"

namespace Zephyr
{
    struct TextureDescription
    {
        uint32_t      width;
        uint32_t      height;
        uint32_t      depth;
        uint32_t      levels;
        uint32_t      samples;
        TextureFormat format;
        TextureUsage  usage;
        SamplerType   sampler;
        PipelineType  pipelines;
        bool          operator==(const TextureDescription& t) const
        {
            return width == t.width && height == t.height && depth == t.depth && levels == t.levels &&
                   samples == t.samples && format == t.format && usage == t.usage && sampler == t.sampler &&
                   pipelines == t.pipelines;
        }
    };

    struct hash_fn_td
    {
        size_t operator()(const TextureDescription& t) const
        {

            return std::hash<uint32_t>()(t.width) ^ std::hash<uint32_t>()(t.height) ^ std::hash<uint32_t>()(t.depth) ^
                   std::hash<uint32_t>()(t.levels) ^ std::hash<uint32_t>()(t.samples) ^
                   std::hash<uint32_t>()((uint32_t)t.format) ^ std::hash<uint32_t>()(t.usage) ^
                   std::hash<uint32_t>()((uint32_t)t.sampler) ^ std::hash<uint32_t>()(t.pipelines);
        }
    };

    struct TextureUpdateDescriptor
    {
        uint32_t layer;
        uint32_t depth;
        uint32_t level;

        void*    data;
        uint32_t size;
    };

    class RHITexture
    {
    public:
        virtual ~RHITexture()             = default;
        virtual TextureFormat GetFormat() = 0;
    };
} // namespace Zephyr