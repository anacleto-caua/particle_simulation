#include "GpuBuffer.hpp"
#include "Core/Types/AppTypes.hpp"
#include <cstddef>
#include <stdexcept>

GpuBuffer::GpuBuffer(
    DeviceContext& deviceCtx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    QueueContext& queue
) : m_deviceCtx(deviceCtx), m_size(size), queueCtx(queue) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_deviceCtx.m_logicalDevice, &bufferInfo, nullptr, &m_vkBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_deviceCtx.m_logicalDevice, m_vkBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findSuitableMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_deviceCtx.m_logicalDevice, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_deviceCtx.m_logicalDevice, m_vkBuffer, m_memory, 0);
}

GpuBuffer::~GpuBuffer() {
    vkDestroyBuffer(m_deviceCtx.m_logicalDevice, m_vkBuffer, nullptr);
    vkFreeMemory(m_deviceCtx.m_logicalDevice, m_memory, nullptr);
}

void GpuBuffer::copyFromCpu(const void *sourceData, size_t size) {
    GpuBuffer stagingBuffer = GpuBuffer(
        m_deviceCtx,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        queueCtx
    );

    size_t sizeCast = static_cast<size_t>(size);

    void* mappedData;
    vkMapMemory(stagingBuffer.m_deviceCtx.m_logicalDevice, stagingBuffer.m_memory, 0, sizeCast, 0, &mappedData);
    memcpy(mappedData, sourceData, sizeCast);
    vkUnmapMemory(stagingBuffer.m_deviceCtx.m_logicalDevice, stagingBuffer.m_memory);

    this->copyFromBuffer(&stagingBuffer, size);
}

void GpuBuffer::copyFromBuffer(GpuBuffer *srcBuffer) {
    copyFromBuffer(srcBuffer, m_size);
}

void GpuBuffer::copyFromBuffer(GpuBuffer *srcBuffer, VkDeviceSize size) {
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    
    m_deviceCtx.executeCommand(
        [&](VkCommandBuffer cmd) {
            vkCmdCopyBuffer(
                cmd, 
                srcBuffer->m_vkBuffer,
                this->m_vkBuffer,
                1,
                &copyRegion
            );
        },
        queueCtx.mainCmdPool,
        queueCtx.queue
    );
}

void GpuBuffer::mapAndWrite(const void* data, VkDeviceSize size) {
    void* mappedData;
    vkMapMemory(m_deviceCtx.m_logicalDevice, m_memory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, (size_t)size);
    vkUnmapMemory(m_deviceCtx.m_logicalDevice, m_memory);
}

uint32_t GpuBuffer::findSuitableMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_deviceCtx.m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (
            (typeFilter & (1 << i)) && 
            ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        ) {
            return i;
        }
    }

    throw std::runtime_error("failed to find a suitable memory type!");
}