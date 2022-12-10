#pragma once
#include "framegraph/FrameGraphResource.h"
#include "pch.h"
#include "rhi/Handle.h"
#include "rhi/RHIRenderTarget.h"
#include "rhi/RHIRenderUnit.h"

namespace Zephyr
{
    struct hash_fn_rt_a_p
    {
        size_t operator()(const std::pair<RenderTargetDescription, std::vector<Handle<RHITexture>>> p) const {
            auto h = hash_fn_rt()(p.first);
            for (auto& a : p.second)
            {
                h ^= a.id;
            }

            return h;
        }
    };

    class Driver;
    class Mesh;
    /*
        manages resources created at render time. e.g. vertex/index buffer, render targets, attachments, etc
    */
    class RenderResourceManager final
    {
    public:
        RenderResourceManager(Driver* driver);
        ~RenderResourceManager() = default;
        void Shutdown();

        Handle<RHITexture>      CreateTexture(const TextureDescription& desc, bool external);
        Handle<RHIRenderTarget> CreateRenderTarget(const RenderTargetDescription&         desc,
                                                   const std::vector<Handle<RHITexture>>& attachments);

        void DestroyTexture(Handle<RHITexture> handle);
        void DestroyRenderTarget(Handle<RHIRenderTarget> handle);

    private:
        Driver*                                                                                m_Driver;
        std::unordered_map<TextureDescription, Handle<RHITexture>, hash_fn_td>                 m_TextureCache;
        std::unordered_map<Handle<RHITexture>, TextureDescription, hash_fn_handle<RHITexture>> m_InUseTexture;
        std::unordered_map<std::pair<RenderTargetDescription, std::vector<Handle<RHITexture>>>,
                           Handle<RHIRenderTarget>,
                           hash_fn_rt_a_p>
                                                                                                              m_RTCache;
        std::unordered_map<Handle<RHIRenderTarget>,
                           std::pair<RenderTargetDescription, std::vector<Handle<RHITexture>>>,
                           hash_fn_handle<RHIRenderTarget>>
            m_InUseRT;
    };
} // namespace Zephyr