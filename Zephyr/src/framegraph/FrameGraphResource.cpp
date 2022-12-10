#include "FrameGraphResource.h"
#include "render/RenderResourceManager.h"

namespace Zephyr
{
    FrameGraphHandle::FrameGraphHandle() : m_ID(INVALID_HANDLE_ID) {}
    FrameGraphHandle::FrameGraphHandle(FrameGraphHandleID id) : m_ID(id) {}

    void FrameGraphTexture::Create(RenderResourceManager* manager, const TextureDescription& desc)
    {
        m_Handle = manager->CreateTexture(desc, m_External);
    }
    void FrameGraphTexture::Destroy(RenderResourceManager* manager)
    {
        assert(m_Handle.IsValid());
        if (m_External)
        {
            return;
        }
        manager->DestroyTexture(m_Handle);
    }
} // namespace Zephyr