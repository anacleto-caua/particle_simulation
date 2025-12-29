#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include "PipelineAttachmentBuilder.hpp"

class RenderPassBuilder {
public:
    VkRenderPassCreateInfo renderPassInfo{};

    // Subpasses
    VkSubpassDependency dependency{};
    std::vector<VkSubpassDescription> subpasses;

    // Attachments
    std::vector<VkAttachmentDescription> descriptions;
    std::vector<VkAttachmentReference> references;

    std::vector<VkAttachmentReference> colorAttachments;

    VkAttachmentReference depthStencil{ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED };
    VkAttachmentReference colorResolve{ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED };

    static RenderPassBuilder setDefaults(
        VkFormat swapChainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat
    ) {
        RenderPassBuilder builder;
        
        builder.dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        builder.dependency.dstSubpass = 0;
        builder.dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        builder.dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        builder.dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        builder.dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
         
        builder.renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        
        VkSubpassDescription mainSubpass{};
        mainSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        builder.subpasses.push_back(mainSubpass);

        builder.depthStencil.attachment = VK_ATTACHMENT_UNUSED;
        builder.colorResolve.attachment = VK_ATTACHMENT_UNUSED;

        return builder;
    }

    RenderPassBuilder addAttachment(VkSubpassDescription subpass) {
        subpasses.push_back(subpass);
        return *this;
    }

    RenderPassBuilder& addDepthStencilAttachment(const PipelineAttachmentBuilder& builder) {
        if(depthStencil.attachment != VK_ATTACHMENT_UNUSED) {
            throw std::runtime_error("trying to redefine the depth stencil!");
        }
        addAttachment(builder);
        depthStencil = references.back();
        return *this;
    }

    RenderPassBuilder& addResolveAttachment(const PipelineAttachmentBuilder& builder) {
        if(colorResolve.attachment != VK_ATTACHMENT_UNUSED) {
            throw std::runtime_error("trying to redefine the color resolve!");
        }
        addAttachment(builder);
        colorResolve = references.back();
        return *this;
    }

    RenderPassBuilder& addColorAttachment(const PipelineAttachmentBuilder& builder) {
        addAttachment(builder);
        colorAttachments.push_back(references.back());
        return *this;
    }

    VkRenderPass build(VkDevice logicalDevice) {
        VkRenderPass renderPass;
        // TODO: See the limitations of such approach
        consolidadeMainSubpass();

        renderPassInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
        renderPassInfo.pAttachments = descriptions.data();

        renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        return renderPass;
    }

private:
    void addAttachment(const PipelineAttachmentBuilder& builder) {
        VkAttachmentDescription description = builder.attachment.description;
        VkAttachmentReference reference = builder.attachment.reference;
        
        descriptions.push_back(description);
        references.push_back(reference);
    }

    void consolidadeMainSubpass() {
        VkSubpassDescription& subpass = subpasses[0];

        subpass.colorAttachmentCount = colorAttachments.size();
        subpass.pColorAttachments = colorAttachments.data();

        if (depthStencil.attachment != VK_ATTACHMENT_UNUSED) {
            subpass.pDepthStencilAttachment = &depthStencil;
        } else {
            subpass.pDepthStencilAttachment = nullptr;
        }
    
        if (colorResolve.attachment != VK_ATTACHMENT_UNUSED) {
            subpass.pResolveAttachments = &colorResolve;
        } else {
            subpass.pResolveAttachments = nullptr;
        }
    }
};