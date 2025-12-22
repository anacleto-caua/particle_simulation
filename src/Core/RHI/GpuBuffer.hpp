#pragma once

#include <vulkan/vulkan.h>
#include "./DeviceContext.hpp"
#include "./Types/AppTypes.hpp"

class GpuBuffer {
public:
    GpuBuffer(DeviceContext& deviceCtx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, QueueContext& queueCtx);
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    void copyFromCpu(const void *sourceData);
    void copyFromCpu(const void *sourceData, size_t size);

    void mapAndWrite(const void* data, VkDeviceSize size);

    void copyFromBuffer(GpuBuffer *srcBuffer);
    void copyFromBuffer(GpuBuffer *srcBuffer, VkDeviceSize size);

    VkBuffer m_vkBuffer;

    QueueContext& m_queueCtx;

private:
    DeviceContext& m_deviceCtx;
    
    VkDeviceMemory m_memory;
    VkDeviceSize m_size;

    uint32_t findSuitableMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};