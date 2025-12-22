#pragma once

#include <optional>
#include <string>
#include <vulkan/vulkan.h>
#include "../RHI/DeviceContext.hpp"
#include "Image.hpp"

class Texture {

public:
    Texture(
        DeviceContext &deviceCtx, 
        const std::string& filepath,
        VkSampleCountFlagBits numSamples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImageAspectFlags aspectFlags
    );
    
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    std::optional<Image> m_image;

private:
    DeviceContext& m_deviceCtx;

    void generateMipmaps();
    void recordGenerateMipmapsCmd(VkCommandBuffer cmd);
};