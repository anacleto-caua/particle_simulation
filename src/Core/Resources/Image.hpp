#pragma once

#include <vulkan/vulkan.h>
#include "Core/RHI/Types/AppTypes.hpp"
#include "Core/RHI/DeviceContext.hpp"
#include "Barrier.hpp"

class Image {
public:
    Image(
        DeviceContext *deviceCtx,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkSampleCountFlagBits numSamples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImageAspectFlags aspectFlags
    );

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;
    
    VkImage m_vkImage;
    VkImageView m_imageView;
    VkDeviceMemory m_imageMemory;

    uint32_t m_mipLevels, m_width, m_height;
    
    VkFormat m_format;

    void memoryBarrier(const BarrierBuilder& builder, const QueueContext &execQueueCtx);
    void memoryBarrier(const BarrierBuilder& builder, VkCommandBuffer commandBuffer);
    
private:
    DeviceContext *m_deviceCtx;

    void destroy();
    
};
