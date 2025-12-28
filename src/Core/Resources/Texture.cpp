#include "Texture.hpp"

#include <stdexcept>

#include <stb_image.h>

#include "Core/RHI/GpuBuffer.hpp"
#include "Image.hpp"

Texture::Texture(
    DeviceContext &deviceCtx,
    const std::string& filepath
) : m_deviceCtx(deviceCtx), m_sampler(deviceCtx.m_textureSampler) {
    int texW, texH, texChannels;
    
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texW, &texH, &texChannels, STBI_rgb_alpha);
    
    uint32_t width = texW;
    uint32_t height = texW;
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    
    VkDeviceSize imageSize = width * height * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    GpuBuffer stagingBuffer = GpuBuffer(
        m_deviceCtx,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_deviceCtx.m_transferQueueCtx
    );

    stagingBuffer.copyFromCpu(pixels, imageSize);
    stbi_image_free(pixels);

    m_image = Image(
        &deviceCtx,
        width,
        height,
        mipLevels,
        VK_SAMPLE_COUNT_1_BIT,  // TODO: Check if I don't need to fetch the msaa numSamples to use here
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Transition the layout from undefined
    m_image->memoryBarrier(BarrierBuilder::transitLayout(
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0,VK_ACCESS_TRANSFER_WRITE_BIT)
        .stages(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)
        .levelCount(mipLevels),
        m_deviceCtx.m_transferQueueCtx
    );

    stagingBuffer.copyBufferToImage(*m_image);
    
    // TODO: Check if I should have a single Command Buffer for this chain of event
    //  or run each barrier as a single time command 
    
    // Release from Transfer Queue
    m_image->memoryBarrier(BarrierBuilder::transitLayout(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,0)
        .queues(m_deviceCtx.m_transferQueueCtx,m_deviceCtx.m_graphicsQueueCtx)
        .stages(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
        .levelCount(mipLevels),
        m_deviceCtx.m_transferQueueCtx
    );

    // Acquire on Graphics Queue
    m_image->memoryBarrier(BarrierBuilder::transitLayout(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0, VK_ACCESS_SHADER_READ_BIT)
        .queues(m_deviceCtx.m_transferQueueCtx,m_deviceCtx.m_graphicsQueueCtx)
        .stages(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
        .levelCount(mipLevels),
        m_deviceCtx.m_graphicsQueueCtx
    );
}

Texture::~Texture() { }

void Texture::generateMipmaps() {
    m_deviceCtx.executeCommand(
        [&](VkCommandBuffer cmd) {
            this->recordGenerateMipmapsCmd(cmd);
        },
        m_deviceCtx.m_graphicsQueueCtx
    );
}

void Texture::recordGenerateMipmapsCmd(VkCommandBuffer cmd) {
    BarrierBuilder baseBarrierBuilder = BarrierBuilder::transitLayout(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
    );

    int32_t mipWidth = m_image->m_width;
    int32_t mipHeight = m_image->m_height;

    for (uint32_t i = 1; i < m_image->m_mipLevels; i++) {
        baseBarrierBuilder.baseMipLevel(i - 1)
            .layouts(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            .accessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
        m_image->memoryBarrier(baseBarrierBuilder, cmd);
        
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd,
            m_image->m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_image->m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        baseBarrierBuilder.layouts(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .accessMasks(VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT);
        m_image->memoryBarrier(baseBarrierBuilder, cmd);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    baseBarrierBuilder.baseMipLevel(m_image->m_mipLevels - 1)
        .layouts(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .accessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    m_image->memoryBarrier(baseBarrierBuilder, cmd);
}
