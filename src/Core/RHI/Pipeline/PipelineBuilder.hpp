#pragma once

#include <vector>

#include <vulkan/vulkan.h>

struct PipelineBuilder {
    std::vector<VkDynamicState> m_dynamicStates{};
    VkPipelineDynamicStateCreateInfo m_dynamicState{};
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages{};
    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{};
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_rasterizer{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
    VkViewport m_viewport{};
    VkRect2D m_scissor{};
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};

    PipelineBuilder& setDefaults();
    
    PipelineBuilder& addShaderStage(VkPipelineShaderStageCreateInfo info);
    
    VkPipeline build(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout);
};