#include "GpuBuffer.hpp"
#include <stdexcept>

GpuBuffer::GpuBuffer(
    DeviceContext& deviceCtx,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    QueueContext& queue
) : m_deviceCtx(deviceCtx), m_size(size), m_queueCtx(queue) {
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
    allocInfo.memoryTypeIndex = m_deviceCtx.findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_deviceCtx.m_logicalDevice, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_deviceCtx.m_logicalDevice, m_vkBuffer, m_memory, 0);
}

GpuBuffer::~GpuBuffer() {
    vkDestroyBuffer(m_deviceCtx.m_logicalDevice, m_vkBuffer, nullptr);
    vkFreeMemory(m_deviceCtx.m_logicalDevice, m_memory, nullptr);
}

void GpuBuffer::copyFromCpu(const void *sourceData) {
    copyFromCpu(sourceData, m_size);
}

void GpuBuffer::copyFromCpu(const void *sourceData, size_t size) {
    GpuBuffer stagingBuffer = GpuBuffer(
        m_deviceCtx,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_queueCtx
    );

    stagingBuffer.mapAndWrite(sourceData, size);
    this->copyFromBuffer(&stagingBuffer);
}

void GpuBuffer::mapAndWrite(const void* data, VkDeviceSize size) {
    void* mappedData;
    vkMapMemory(m_deviceCtx.m_logicalDevice, m_memory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, (size_t)size);
    vkUnmapMemory(m_deviceCtx.m_logicalDevice, m_memory);
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
        m_queueCtx
    );
}

void GpuBuffer::copyBufferToImage(Image &image) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        image.m_width,
        image.m_height,
        1
    };

    m_deviceCtx.executeCommand(
        [&](VkCommandBuffer cmd) {
            vkCmdCopyBufferToImage(
                cmd,
                m_vkBuffer,
                image.m_vkImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        },
        m_queueCtx
    );
}