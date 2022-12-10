#pragma once
#include "rhi/RHIEnums.h"
#include "rhi/Handle.h"
#include "rhi/RHIShaderSet.h"

namespace Zephyr
{
    class Driver;
    class ShaderSet final
    {
    public:
        inline Handle<RHIShaderSet> GetHandle() const { return m_Handle; }

    private:
        ShaderSet(const ShaderSetDescription& desc, Driver* driver);
        void Destroy(Driver* driver);

        ~ShaderSet() = default;


    private:
        ShaderSetDescription m_Description;
        Handle<RHIShaderSet> m_Handle;
        friend class ResourceManager;
    };
}