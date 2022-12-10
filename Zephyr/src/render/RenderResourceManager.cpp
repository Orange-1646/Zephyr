#include "RenderResourceManager.h"
#include "resource/Mesh.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    RenderResourceManager::RenderResourceManager(Driver* driver) : m_Driver(driver) {}

    void RenderResourceManager::Shutdown() { 
        // there should be no texture/rt in use
        assert(m_InUseTexture.size() == 0 && m_InUseRT.size() == 0);
        for (auto& texture : m_TextureCache)
        {
            m_Driver->DestroyTexture(texture.second);
        }
        for (auto& rt : m_RTCache)
        {
            m_Driver->DestroyRenderTarget(rt.second);
        }
    }

    Handle<RHITexture> RenderResourceManager::CreateTexture(const TextureDescription& desc, bool external)
    {
        Handle<RHITexture> handle;
        if (external)
        {
            return m_Driver->GetSwapchainImage();
        }
        auto               iter = m_TextureCache.find(desc);
        if (iter != m_TextureCache.end())
        {
            handle = iter->second;
            m_TextureCache.erase(iter);
        }
        else
        {
            handle = m_Driver->CreateTexture(desc);
        }

        m_InUseTexture.insert({handle, desc});

        return handle;
    }

    Handle<RHIRenderTarget>
    RenderResourceManager::CreateRenderTarget(const RenderTargetDescription&         desc,
                                              const std::vector<Handle<RHITexture>>& attachments)
    {
        Handle<RHIRenderTarget> handle;

        std::pair<RenderTargetDescription, std::vector<Handle<RHITexture>>> p = {desc, attachments};

        auto iter = m_RTCache.find(p);
        if (iter != m_RTCache.end())
        {
            handle = iter->second;
            m_RTCache.erase(iter);
        }
        else
        {
            handle = m_Driver->CreateRenderTarget(desc, attachments);
        }

        m_InUseRT.insert({handle, p});
        return handle;
    }

    // this actually caches the handle instead of destroying it
    void RenderResourceManager::DestroyTexture(Handle<RHITexture> handle)
    {
        auto iter = m_InUseTexture.find(handle);
        assert(iter != m_InUseTexture.end());

        m_TextureCache.insert({iter->second, iter->first});
        m_InUseTexture.erase(iter);
    }

    void RenderResourceManager::DestroyRenderTarget(Handle<RHIRenderTarget> handle)
    {
        auto iter = m_InUseRT.find(handle);
        assert(iter != m_InUseRT.end());

        m_RTCache.insert({iter->second, iter->first});
        m_InUseRT.erase(iter);
    }
} // namespace Zephyr