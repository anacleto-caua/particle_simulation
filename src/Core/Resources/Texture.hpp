#pragma once

#include <string>
#include <optional>

#include <vulkan/vulkan.h>

#include "Core/RHI/DeviceContext.hpp"
#include "Image.hpp"

class Texture {

public:
    Texture(
        DeviceContext &deviceCtx, 
        const std::string& filepath
    );
    
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    std::optional<Image> m_image;
    
    VkSampler& m_sampler;

    void generateMipmaps();

private:
    DeviceContext& m_deviceCtx;

    void recordGenerateMipmapsCmd(VkCommandBuffer cmd);
};