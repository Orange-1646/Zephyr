#include "Buffer.h"
#include "rhi/Driver.h"

namespace Zephyr
{

    Buffer::Buffer(const BufferDescription& desc, Driver* driver) : m_Description(desc)
    {
        m_Handle = driver->CreateBuffer(desc);
    }

    void Buffer::Destroy(Driver* driver) { driver->DestroyBuffer(m_Handle); }
} // namespace Zephyr