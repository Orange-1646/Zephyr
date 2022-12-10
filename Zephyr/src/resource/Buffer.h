#pragma once
#include "rhi/RHIBuffer.h"
#include "rhi/Handle.h"

namespace Zephyr
{
    class Driver;
    class Buffer
    {
    public:
        inline Handle<RHIBuffer> GetHandle() { return m_Handle; }
    private:
        Buffer(const BufferDescription& desc, Driver* driver);
        virtual ~Buffer() = default;
        void Destroy(Driver* driver);
    private:
        BufferDescription m_Description;
        Handle<RHIBuffer> m_Handle;

        friend class ResourceManager;
    };
}