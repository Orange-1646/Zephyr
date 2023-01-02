#pragma once
#include "VulkanCommon.h"
#include "VulkanContext.h"
#include "rhi/RHIBuffer.h"
#include "rhi/RHIEnums.h"

namespace Zephyr
{
    class VulkanBuffer : public RHIBuffer
    {
    public:
        VulkanBuffer(VulkanDriver* driver, const BufferDescription& desc);
        ~VulkanBuffer() override = default;

        void Destroy(VulkanDriver* driver);
        void Update(VulkanDriver* driver, const BufferUpdateDescriptor& desc);

        inline uint32_t GetRange() const
        {
            assert(m_Description.size != 0);
            return m_Description.size;
        }

        inline VkBuffer       GetBuffer() const { return m_Buffer; }
        inline uint32_t       GetOffset(uint32_t index) const { return m_RingBufferAlignment * index; }
        inline VkDeviceMemory GetMemory() const { return m_Memory; }
        inline void*          GetMapped() { return m_DataPtr; }

    private:
        uint32_t GetBufferSize(VulkanDriver* driver, uint32_t size, BufferUsage usage, BufferMemoryType type);

    private:
        VkBuffer       m_Buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;

        BufferDescription m_Description;
        // when the buffer is a ringbuffer, the actual size is x times the normal size,
        // where x is the maximun concurrent frame
        uint32_t m_RingBufferSize      = 0;
        uint32_t m_RingBufferAlignment = 0;

        bool  m_Mapped  = false;
        void* m_DataPtr = nullptr;

        friend class VulkanTexture;
    };
} // namespace Zephyr