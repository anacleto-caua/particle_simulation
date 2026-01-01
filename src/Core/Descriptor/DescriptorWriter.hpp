#pragma once

#include "Core/RHI/GpuBuffer.hpp"
#include "Core/Resources/Texture.hpp"
#include <deque>
#include <vector>
#include <vulkan/vulkan.h>

class DescriptorWriter {
public:
    void addStorageBufferBinding(VkDescriptorSet& dstSet, const uint32_t dst, const GpuBuffer& buffer, const uint32_t count = 1) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer.m_vkBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.m_size;

        bufferInfos.push_back(bufferInfo);

        VkWriteDescriptorSet descriptorConfig = addBinding(dstSet, dst, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
        descriptorConfig.pBufferInfo = &bufferInfos.back();

        writeSets.push_back(descriptorConfig);
    }

    void addUniformBufferBinding(VkDescriptorSet& dstSet, const uint32_t dst, const GpuBuffer& buffer, const uint32_t count = 1) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer.m_vkBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.m_size;

        bufferInfos.push_back(bufferInfo);

        VkWriteDescriptorSet descriptorConfig = addBinding(dstSet, dst, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
        descriptorConfig.pBufferInfo = &bufferInfos.back();

        writeSets.push_back(descriptorConfig);
    }

    void addImageBinding(VkDescriptorSet& dstSet, const uint32_t dst, const Texture& texture, const VkImageLayout imageLayout, const uint32_t count = 1) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = imageLayout;
        imageInfo.imageView = texture.m_image->m_imageView;
        imageInfo.sampler = texture.m_sampler;

        imageInfos.push_back(imageInfo);

        VkWriteDescriptorSet descriptorConfig = addBinding(dstSet, dst, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count);
        descriptorConfig.pImageInfo = &imageInfos.back();;

        writeSets.push_back(descriptorConfig);
    }

    void writeAll(VkDevice& logicalDevice) {
        vkUpdateDescriptorSets(logicalDevice, writeSets.size(), writeSets.data(), 0, nullptr);
    }

private:
    std::vector<VkWriteDescriptorSet> writeSets;

    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::deque<VkDescriptorImageInfo> imageInfos;
    
    VkWriteDescriptorSet addBinding(VkDescriptorSet& dstSet, const uint32_t dst, const VkDescriptorType type, const uint32_t count) {
        VkWriteDescriptorSet descriptorConfig{};
        descriptorConfig.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorConfig.dstSet = dstSet;
        descriptorConfig.dstBinding = dst;
        descriptorConfig.descriptorType = type;
        descriptorConfig.descriptorCount = count;
        return descriptorConfig;
    }
};