#pragma once
#include "FrameGraphResource.h"
#include "pch.h"
#include "render/RenderResourceManager.h"
#include "rhi/RHITexture.h"

namespace Zephyr
{
    class PassNode;
    class VirtualResourceBase
    {
    public:
        VirtualResourceBase() {}
        VirtualResourceBase(VirtualResourceBase* parent) : m_Parent(parent) {}

        virtual void Devirtualize(RenderResourceManager* manager) = 0;
        virtual void Destroy(RenderResourceManager* manager)      = 0;

        bool IsSubresource() const { return m_Parent != nullptr; }
        bool IsCulled() const { return m_Refcount == 0; }
        void AddRef() { m_Refcount++; }
        void DecRef() { m_Refcount--; }

        inline PassNode* First() { return m_First; }
        inline PassNode* Last() { return m_Last; }

        VirtualResourceBase* GetAncestor()
        {
            VirtualResourceBase* ptr = this;

            while (ptr->m_Parent != nullptr)
            {
                ptr = ptr->m_Parent;
            }

            return ptr;
        }

        void AddPassDependency(PassNode* node)
        {
            AddRef();
            if (!m_First)
            {
                m_First = node;
            }
            m_Last = node;
        }

    protected:
        VirtualResourceBase* m_Parent = nullptr;

        PassNode* m_First    = nullptr;
        PassNode* m_Last     = nullptr;
        uint32_t  m_Refcount = 0;
    };

    // a virtual resource holds a reference to the framegraph resource and a subresource descriptor.
    // when the framegraph is done with culling, devirtualize will be called and the resource will be instantiated
    class VirtualResource : public VirtualResourceBase
    {
        using Descriptor           = typename FrameGraphTexture::Descriptor;
        using SubresourceDesciptor = typename FrameGraphTexture::SubresourceDescriptor;

    public:
        VirtualResource(const TextureDescription& desc, bool external = false) :
            m_Descriptor(desc), m_Resource(external)
        {}
        VirtualResource(VirtualResourceBase* parent, const SubresourceDesciptor& sub) :
            VirtualResourceBase(parent), m_SubresourceDescriptor(sub)
        {}
        void Devirtualize(RenderResourceManager* manager) override
        {
            if (IsSubresource())
            {
                return;
            }
            m_Resource.Create(manager, m_Descriptor);
        }
        void Destroy(RenderResourceManager* manager) override
        {
            if (IsSubresource())
            {
                return;
            }
            m_Resource.Destroy(manager);
        }

        Handle<RHITexture> GetRHITexture()
        {
            if (IsSubresource())
            {
                return static_cast<VirtualResource*>(m_Parent)->GetRHITexture();
            }
            return m_Resource.GetHandle();
        }

        ViewRange GetViewRange()
        {
            if (IsSubresource())
            {
                return {m_SubresourceDescriptor.baseLayer,
                        m_SubresourceDescriptor.layerCount,
                        m_SubresourceDescriptor.baseLevel,
                        m_SubresourceDescriptor.levelCount};
            }
            else
            {
                return {0, ALL_LAYERS, 0, ALL_LEVELS};
            }
        }

        //
        AttachmentDescriptor GetAttachmentDescriptor()
        {
            // the assumption is that if used as a render attachment, we can only use one layer and one level at a time
            AttachmentDescriptor desc {};
            if (IsSubresource())
            {
                desc.layer = m_SubresourceDescriptor.baseLayer;
                desc.level = m_SubresourceDescriptor.baseLevel;
            }
            else
            {
                desc.layer = 0;
                desc.level = 0;
            }

            return desc;
        }

        inline TextureUsage GetUsage() const { return m_Descriptor.usage; }

    private:
        FrameGraphTexture    m_Resource;
        TextureDescription   m_Descriptor {};
        SubresourceDesciptor m_SubresourceDescriptor {};
    };
} // namespace Zephyr