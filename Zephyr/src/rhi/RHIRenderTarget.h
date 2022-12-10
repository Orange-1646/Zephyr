#pragma once
#include "Handle.h"
#include "RHITexture.h"

namespace Zephyr
{
    struct AttachmentDescriptor
    {
        // Handle<RHITexture> texture;
        uint32_t     layer;
        uint32_t     level;
        TextureUsage usage;
        bool         clear = true;
        bool         save  = true;
        bool         operator==(const AttachmentDescriptor& rhs) const
        {
            return layer == rhs.layer && level == rhs.level && usage == rhs.usage && clear == rhs.clear &&
                   save == rhs.save;
        }
    };

    struct RenderTargetDescription
    {
        std::vector<AttachmentDescriptor> color;
        bool                              useDepthStencil;
        AttachmentDescriptor              depthStencil;
        bool                              present = false;

        bool operator==(const RenderTargetDescription& rhs) const
        {
            if (rhs.present != present)
            {
                return false;
            }

            if (rhs.color.size() != color.size())
            {
                return false;
            }
            if (useDepthStencil != rhs.useDepthStencil)
            {
                return false;
            }
            if (!(depthStencil == rhs.depthStencil))
            {
                return false;
            }

            for (uint32_t i = 0; i < color.size(); i++)
            {
                if (!(color[i] == rhs.color[i]))
                {
                    return false;
                }
            }
            return true;
        }
    };

    struct hash_fn_rt
    {
        size_t operator()(const RenderTargetDescription& t) const
        {
            size_t r = 0;
            for (auto& color : t.color)
            {
                r ^= std::hash<uint32_t>()(color.layer) ^ std::hash<uint32_t>()(color.level) ^
                     std::hash<uint32_t>()(color.usage) ^ std::hash<bool>()(color.clear) ^
                     std::hash<bool>()(color.save);
            }
            r ^= std::hash<bool>()(t.useDepthStencil) ^ std::hash<uint32_t>()(t.depthStencil.layer) ^
                 std::hash<uint32_t>()(t.depthStencil.level) ^ std::hash<uint32_t>()(t.depthStencil.usage) ^
                 std::hash<bool>()(t.depthStencil.clear) ^ std::hash<bool>()(t.depthStencil.save);

            return r;
        }
    };

    class RHIRenderTarget
    {
    public:
        RHIRenderTarget()          = default;
        virtual ~RHIRenderTarget() = default;
    };
} // namespace Zephyr