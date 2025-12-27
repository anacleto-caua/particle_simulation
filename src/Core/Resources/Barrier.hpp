#pragma once

#include <vulkan/vulkan.h>
#include "Core/RHI/Types/AppTypes.hpp"

struct BarrierConfig {
    VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED; 
    uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED; 

    VkAccessFlags srcAccessMask = 0;
    VkAccessFlags dstAccessMask = 0;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; 
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    uint32_t baseMipLevel = 0;

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    uint32_t levelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;
};

class BarrierBuilder {
public:
    BarrierConfig config;

    static BarrierBuilder transitLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        BarrierBuilder builder;
        builder.config.oldLayout = oldLayout;
        builder.config.newLayout = newLayout;
        builder.config.srcAccessMask = srcAccessMask;
        builder.config.dstAccessMask = dstAccessMask;
        return builder;
    }

    BarrierBuilder& queues(const QueueContext &srcQueueCtx, const QueueContext &dstQueueCtx) {
        config.srcQueueFamily = srcQueueCtx.queueFamilyIndex;
        config.dstQueueFamily = dstQueueCtx.queueFamilyIndex;
        return *this;
    }

    BarrierBuilder& stages(VkPipelineStageFlags src, VkPipelineStageFlags dst) {
        config.srcStage = src;
        config.dstStage = dst;
        return *this;
    }

    BarrierBuilder& baseMipLevel(uint32_t baseMipLevel) {
        config.baseMipLevel = baseMipLevel;
        return *this;
    }

    BarrierBuilder& aspectMask(VkImageAspectFlags aspectMask) {
        config.aspectMask = aspectMask;
        return *this;
    }

    BarrierBuilder& levelCount(uint32_t levelCount) {
        config.levelCount = levelCount;
        return *this;
    }

    BarrierBuilder& baseArrayLayer(uint32_t baseArrayLayer) {
        config.baseArrayLayer = baseArrayLayer;
        return *this;
    }

    BarrierBuilder& layerCount(uint32_t layerCount) {
        config.layerCount = layerCount;
        return *this;
    }

    // Rewrites
    BarrierBuilder& layouts(VkImageLayout oldLayout, VkImageLayout newLayout) {
        config.oldLayout = oldLayout;
        config.newLayout = newLayout;
        return *this;
    }

    BarrierBuilder& accessMasks(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        config.srcAccessMask = srcAccessMask;
        config.dstAccessMask = dstAccessMask;
        return *this;
    }
};