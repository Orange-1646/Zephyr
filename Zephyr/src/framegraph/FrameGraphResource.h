#pragma once
#include "rhi/RHIEnums.h"
#include "rhi/Handle.h"
#include "rhi/RHITexture.h"

namespace Zephyr
{
    class RenderResourceManager;

    using FrameGraphHandleID = uint32_t;
    class FrameGraphHandle
    {
    public:
        static inline constexpr FrameGraphHandleID INVALID_HANDLE_ID = UINT32_MAX;
        static bool IsValid(const FrameGraphHandle& handle) { return handle.m_ID != INVALID_HANDLE_ID; }

        FrameGraphHandle();
        FrameGraphHandle(FrameGraphHandleID id);
        virtual ~FrameGraphHandle() = default;
        inline FrameGraphHandleID GetID() const { return m_ID; }

    protected:
        FrameGraphHandleID m_ID;
    };

    template<typename T>
    class FrameGraphResourceHandle : public FrameGraphHandle
    {
    public:
        FrameGraphResourceHandle(const FrameGraphResourceHandle<T>& rhs) : FrameGraphHandle(rhs.GetID()) {}
        FrameGraphResourceHandle() : FrameGraphHandle() {}
        FrameGraphResourceHandle(FrameGraphHandleID id) : FrameGraphHandle(id) {}
        ~FrameGraphResourceHandle() override = default;
    };

    class FrameGraphResource
    {};
    // a framegraph resource is a handle to the actual backend resource.
    // the backend resource will be created when the frame graph finished culling unused node and begin executing
    // the backend resource will be destroyed(internally cached) when the execution finishes
    class FrameGraphTexture : public FrameGraphResource
    {
    public:
        struct Descriptor
        {
            uint32_t      width;
            uint32_t      height;
            uint32_t      depth;
            uint32_t      level;
            uint32_t      samples;
            SamplerType   type;
            TextureFormat format;
        };

        struct SubresourceDescriptor
        {
            uint32_t baseLayer;
            uint32_t layerCount;
            uint32_t baseLevel;
            uint32_t levelCount;
        };

        FrameGraphTexture(bool external = false) : m_External(external) {};
        ~FrameGraphTexture() = default;

        void Create(RenderResourceManager* manager, const TextureDescription& desc);
        void Destroy(RenderResourceManager* manager);

        inline Handle<RHITexture> GetHandle() { return m_Handle; }
    private:
        Handle<RHITexture> m_Handle;
        bool               m_External;
    };
} // namespace Zephyr