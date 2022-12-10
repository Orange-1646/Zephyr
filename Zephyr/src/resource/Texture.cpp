#include "Texture.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    void Texture::Update(const TextureUpdateDescriptor& desc, Driver* driver) { driver->UpdateTexture(desc, m_Handle);
    }

    Texture::Texture(const TextureDescription& desc, Driver* driver) : m_Description(desc)
    {
        m_Handle = driver->CreateTexture(desc);
    }
    void Texture::Destroy(Driver* driver) { driver->DestroyTexture(m_Handle); }
} // namespace Zephyr