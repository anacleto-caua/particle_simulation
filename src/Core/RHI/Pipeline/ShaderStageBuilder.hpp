#pragma once

#include <string>
#include <vector>
#include <fstream>

#include <vulkan/vulkan.h>

// TODO: The interaction between this and PipelineBuilder is far from ideal, please make it more decent
namespace ShaderStageBuilder {
    static VkPipelineShaderStageCreateInfo createShaderStage(const VkDevice &logicalDevice, VkShaderStageFlagBits stage, const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file! " + filename);
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> shaderCode(fileSize);

        file.seekg(0);
        file.read(shaderCode.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        VkShaderModule shaderModule{};
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        VkPipelineShaderStageCreateInfo shaderStage{};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = stage;
        shaderStage.module = shaderModule;
        shaderStage.pName = "main";

        return shaderStage;
    }
};