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

    inline constexpr uint32_t ALL_LAYERS = UINT32_MAX - 1;
    inline constexpr uint32_t ALL_LEVELS = UINT32_MAX - 1;

    struct ViewRange
    {
        uint32_t baseLayer;
        uint32_t layerCount;
        uint32_t baseLevel;
        uint32_t levelCount;

        static ViewRange INVALID_RANGE;

        bool operator==(const ViewRange& rhs) const
        {
            return rhs.baseLayer == baseLayer && rhs.layerCount == layerCount && rhs.baseLevel == baseLevel &&
                   rhs.levelCount == levelCount;
        }

        bool IsValid() { return !(*this == INVALID_RANGE); }
    };


    struct ViewRangeHasher
    {
        std::size_t operator()(const ViewRange& range) const
        {
            using std::hash;
            using std::size_t;
            using std::string;

            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return ((hash<uint32_t>()(range.baseLayer) ^ (hash<uint32_t>()(range.layerCount) << 1)) >> 1) ^
                   (hash<uint32_t>()(range.baseLevel) << 1) ^ (hash<uint32_t>()(range.levelCount) << 1) >> 1;
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