#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDriver.h"
#include "VulkanUtil.h"

namespace Zephyr
{
    VulkanBuffer::VulkanBuffer(VulkanDriver* driver, const BufferDescription& desc) : m_Description(desc)
    {
        assert(desc.size != 0);
        auto& context = *driver->GetContext();
        // create buffer handle
        std::vector<uint32_t> queueIndices;
        if (desc.pipelines & PipelineTypeBits::Graphics)
        {
            queueIndices.push_back(context.QueueIndices().graphics);
        }
        if (desc.pipelines & PipelineTypeBits::Compute)
        {
            queueIndices.push_back(context.QueueIndices().compute);
        }
        VkBufferCreateInfo createInfo {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size                  = GetBufferSize(driver, desc.size, desc.memoryType);
        createInfo.usage                 = VulkanUtil::GetVulkanBufferUsage(desc.usage);
        createInfo.sharingMode           = VulkanUtil::GetSharingMode(desc.pipelines);
        createInfo.queueFamilyIndexCount = queueIndices.size();
        createInfo.pQueueFamilyIndices   = queueIndices.data();

        VK_CHECK(vkCreateBuffer(context.Device(), &createInfo, nullptr, &m_Buffer), "Buffer Creation");

        // allocate device memory
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(context.Device(), m_Buffer, &memoryRequirements);

        uint32_t requiredSize = memoryRequirements.size;
        uint32_t memoryTypeIndex =
            VulkanUtil::FindMemoryTypeIndex(context.PhysicalDevice(),
                                            memoryRequirements.memoryTypeBits,
                                            VulkanUtil::GetBufferMemoryProperties(m_Description.memoryType));

        VkMemoryAllocateInfo memoryAllocateInfo {};
        memoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize  = requiredSize;
        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

        VK_CHECK(vkAllocateMemory(context.Device(), &memoryAllocateInfo, nullptr, &m_Memory),
                 "Buffer Memory Allocation");

        vkBindBufferMemory(context.Device(), m_Buffer, m_Memory, 0);

        // if host visible, map gpu memory into ptr handle
        if (desc.memoryType == BufferMemoryType::Dynamic)
        {
            m_Mapped = true;
            vkMapMemory(context.Device(), m_Memory, 0, desc.size, 0, &m_DataPtr);
        }
        else if (desc.memoryType == BufferMemoryType::DynamicRing)
        {
            m_Mapped = true;
            vkMapMemory(context.Device(), m_Memory, 0, m_RingBufferSize, 0, &m_DataPtr);
        }
    }

    void VulkanBuffer::Destroy(VulkanDriver* driver)
    {
        auto& context = *driver->GetContext();
        if (m_Mapped)
        {
            vkUnmapMemory(context.Device(), m_Memory);
        }
        vkFreeMemory(context.Device(), m_Memory, nullptr);
        vkDestroyBuffer(context.Device(), m_Buffer, nullptr);
    }

    void VulkanBuffer::Update(VulkanDriver* driver, const BufferUpdateDescriptor& desc)
    {
        auto& context = *driver->GetContext();
        if (m_Description.memoryType == BufferMemoryType::Dynamic)
        {
            memcpy((uint8_t*)m_DataPtr + desc.dstOffset, (uint8_t*)desc.data + desc.srcOffset, desc.size);
        }
        else if (m_Description.memoryType == BufferMemoryType::DynamicRing)
        {
            memcpy((uint8_t*)m_DataPtr + desc.dstOffset + driver->GetCurrentFrameIndex() * m_RingBufferAlignment,
                   (uint8_t*)desc.data + desc.srcOffset,
                   desc.size);
        }
        else if (m_Description.memoryType == BufferMemoryType::Static)
        {
            BufferDescription stageDesc {};
            stageDesc.memoryType = BufferMemoryType::Dynamic;
            stageDesc.pipelines  = m_Description.pipelines;
            stageDesc.shaderStages = m_Description.shaderStages;
            stageDesc.size         = desc.size;
            stageDesc.usage        = BufferUsageBits::None;
            VulkanBuffer stage(driver, stageDesc);

            stage.Update(driver, {0, desc.srcOffset, desc.data, desc.size});

            auto cb = driver->BeginSingleTimeCommandBuffer();

            VkBufferCopy region {};
            region.size = desc.size;
            region.srcOffset = 0;
            region.dstOffset = desc.dstOffset;

            vkCmdCopyBuffer(cb, stage.m_Buffer, m_Buffer, 1, &region);

            driver->EndSingleTimeCommandBuffer(cb);

            stage.Destroy(driver);
        }
        else
        {
            assert(false);
        }
    }

    uint32_t VulkanBuffer::GetBufferSize(VulkanDriver* driver, uint32_t size, BufferMemoryType type)
    {
        auto& context = *driver->GetContext();
        if (type != BufferMemoryType::DynamicRing)
        {
            return size;
        }
        // get ringbuffer alignment requirement and actual buffer size
        VkPhysicalDeviceProperties physicalDeviceProperties;

        vkGetPhysicalDeviceProperties(context.PhysicalDevice(), &physicalDeviceProperties);
        uint32_t dynamicAlignment = physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;

        m_RingBufferAlignment = (size + dynamicAlignment - 1) & ~(dynamicAlignment - 1);
        m_RingBufferSize      = MAX_CONCURRENT_FRAME * m_RingBufferAlignment;

        return m_RingBufferSize;
    }

} // namespace Zephyr