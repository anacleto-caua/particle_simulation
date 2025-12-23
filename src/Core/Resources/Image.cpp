#include "Image.hpp"
#include <stdexcept>

Image::Image(
    DeviceContext *deviceCtx, 
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    VkSampleCountFlagBits numSamples,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImageAspectFlags aspectFlags)
{
    m_deviceCtx = deviceCtx;
    m_width = width;
    m_height = height;
    m_mipLevels = mipLevels;
    m_format = format;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_deviceCtx->m_logicalDevice, &imageInfo, nullptr, &m_vkImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_deviceCtx->m_logicalDevice, m_vkImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_deviceCtx->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_deviceCtx->m_logicalDevice, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_deviceCtx->m_logicalDevice, m_vkImage, m_imageMemory, 0);
}

Image::Image(Image&& other) noexcept : m_deviceCtx(other.m_deviceCtx) {
    m_vkImage = other.m_vkImage;
    m_imageView = other.m_imageView;
    m_imageMemory = other.m_imageMemory;
    m_width = other.m_width;
    m_height = other.m_height;
    m_mipLevels = other.m_mipLevels;

    other.m_vkImage = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_imageMemory = VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        destroy();

        m_deviceCtx = other.m_deviceCtx;
        m_vkImage = other.m_vkImage;
        m_imageView = other.m_imageView;
        m_imageMemory = other.m_imageMemory;
        m_width = other.m_width;
        m_height = other.m_height;
        m_mipLevels = other.m_mipLevels;

        other.m_vkImage = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_imageMemory = VK_NULL_HANDLE;
    }
    
    return *this;
}

Image::~Image() {
    destroy();
}

void Image::destroy() {
    if (m_deviceCtx) { 
        VkDevice device = m_deviceCtx->m_logicalDevice;
        if (m_imageView) vkDestroyImageView(device, m_imageView, nullptr);
        if (m_vkImage) vkDestroyImage(device, m_vkImage, nullptr);
        if (m_imageMemory) vkFreeMemory(device, m_imageMemory, nullptr);
    }
}