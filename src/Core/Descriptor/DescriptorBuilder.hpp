#pragma once

#include <vulkan/vulkan.h>

class DescriptorBuilder {
public:
    VkWriteDescriptorSet descriptorConfig{};
    
    // TODO: Consider breaking this into 2 classes, one for image and the other for buffer
    VkDescriptorBufferInfo bufferInfo{};
    VkDescriptorImageInfo imageInfo{};


    static DescriptorBuilder startConfig(const VkDescriptorSet& destinationSet) {
        DescriptorBuilder builder;
        builder.descriptorConfig.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        builder.descriptorConfig.dstSet = destinationSet;
        builder.descriptorConfig.dstArrayElement = 0;
        return builder;
    }

    DescriptorBuilder& addBinding(const uint32_t dst, const VkDescriptorType type) {
        descriptorConfig.dstBinding = dst;
        descriptorConfig.descriptorType = type;
        descriptorConfig.descriptorCount = 1;
        return *this;
    }

    DescriptorBuilder& addUniformBufferBinding(const uint32_t dst, const VkDescriptorBufferInfo bufferInfo) {
        this->addBinding(dst, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        descriptorConfig.descriptorCount = 1;
        this->bufferInfo = bufferInfo;
        descriptorConfig.pBufferInfo = &this->bufferInfo;
        return *this;
    }

    DescriptorBuilder& addImageBinding(const uint32_t dst, const VkDescriptorImageInfo imageInfo) {
        this->addBinding(dst, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        descriptorConfig.descriptorCount = 1;
        this->imageInfo = imageInfo;
        descriptorConfig.pImageInfo = &this->imageInfo;
        return *this;
    }

    // TODO: Consider making it more flexible so multiple Descriptors 
    // can be created with the same vulkan command
    void build(VkDevice logicalDevice) {
        vkUpdateDescriptorSets(logicalDevice, 1, &descriptorConfig, 0, nullptr);
    }
};