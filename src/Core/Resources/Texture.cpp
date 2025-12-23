#include "Texture.hpp"
#include <cstdint>
#include <stb_image.h>
#include <stdexcept>
#include "Core/RHI/GpuBuffer.hpp"

Texture::Texture(
    DeviceContext &deviceCtx,
    const std::string& filepath
) : m_deviceCtx(deviceCtx) {
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

    stagingBuffer.copyBufferToImage(*m_image);

    // Transition the layout from undefined
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        // We are NOT transferring ownership yet, just changing layout.
        // So we keep it ignored (which implies "same queue family").
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = m_image->m_vkImage;;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        m_deviceCtx.executeCommand(
            [&](VkCommandBuffer cmd){
                vkCmdPipelineBarrier(
                    cmd,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            },
            m_deviceCtx.m_transferQueueCtx
        );
    }

    // PART A: Release from Transfer Queue
    // We use the Transfer Pool and Transfer Queue here
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // This transfers ownership:
        barrier.srcQueueFamilyIndex = m_deviceCtx.m_transferQueueCtx.queueFamilyIndex;
        barrier.dstQueueFamilyIndex = m_deviceCtx.m_graphicsQueueCtx.queueFamilyIndex;

        barrier.image = m_image->m_vkImage;;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // RELEASE BARRIER RULES:
        // 1. Src Access: What we just did (Transfer Write)
        // 2. Dst Access: 0 (Ignored during release)
        // 3. Dst Stage: Bottom of Pipe (Finish all transfer work)
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        m_deviceCtx.executeCommand(
            [&](VkCommandBuffer cmd){
                vkCmdPipelineBarrier(
                    cmd,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            },
            m_deviceCtx.m_transferQueueCtx
        );
    }

    // PART B: Acquire on Graphics Queue
    // We use the Graphics Pool and Graphics Queue here
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Indices must match the release exactly
        barrier.srcQueueFamilyIndex = m_deviceCtx.m_transferQueueCtx.queueFamilyIndex;
        barrier.dstQueueFamilyIndex = m_deviceCtx.m_graphicsQueueCtx.queueFamilyIndex;

        barrier.image = m_image->m_vkImage;;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // ACQUIRE BARRIER RULES:
        // 1. Src Access: 0 (Ignored during acquire)
        // 2. Dst Access: What we want to do next (Shader Read)
        // 3. Src Stage: Top of Pipe (Wait for release to become visible)
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        m_deviceCtx.executeCommand(
            [&](VkCommandBuffer cmd){
                vkCmdPipelineBarrier(
                    cmd,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            },
            m_deviceCtx.m_graphicsQueueCtx
        );
    }
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
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_image->m_vkImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;


    int32_t mipWidth = m_image->m_width;
    int32_t mipHeight = m_image->m_height;

    for (uint32_t i = 1; i < m_image->m_mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        
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

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_image->m_mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}
