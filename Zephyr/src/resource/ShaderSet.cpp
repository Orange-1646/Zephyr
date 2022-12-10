#include "ShaderSet.h"
#include "rhi/Driver.h"

namespace Zephyr
{
    ShaderSet::ShaderSet(const ShaderSetDescription& desc, Driver* driver) : m_Description(desc)
    {
        m_Handle = driver->CreateShaderSet(desc);
    }

    void ShaderSet::Destroy(Driver* driver) { driver->DestroyShaderSet(m_Handle); }
} // namespace Zephyr