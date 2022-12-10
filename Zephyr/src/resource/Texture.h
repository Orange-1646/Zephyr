#pragma once
#include "pch.h"
#include "rhi/RHITexture.h"
#include "rhi/Handle.h"

namespace Zephyr
{
    class Driver;


    class Texture
    {
    public:
        void Update(const TextureUpdateDescriptor& desc, Driver* driver);
        inline Handle<RHITexture> GetHandle() const { return m_Handle; } 
    private:
        Texture(const TextureDescription& desc, Driver* driver);
        virtual ~Texture() = default;
        void Destroy(Driver* driver);

    private:
        TextureDescription m_Description;
        Handle<RHITexture> m_Handle;

        friend class ResourceManager;
    };
}