#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Core/RHI/Types/AppTypes.hpp"

struct VulkanContext; 

class DeviceContext {
public:
    DeviceContext(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> requiredDeviceExtensions, bool enableValidationLayers, std::vector<const char *> validationLayers);

    ~DeviceContext();

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;
    
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_logicalDevice = VK_NULL_HANDLE;

    VkSampler m_textureSampler;
    
    QueueContext m_graphicsQueueCtx;
    QueueContext m_transferQueueCtx;
    QueueContext m_presentQueueCtx;
    QueueContext m_computeQueueCtx;

    std::vector<const char*> m_requiredDeviceExtensions;
    
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void executeCommand(const std::function<void(VkCommandBuffer)> &recorder, const QueueContext &queueCtx);
    void executeCommand(const std::function<void(VkCommandBuffer)> &recorder, const QueueContext &queueCtx, VkCommandPool cmdPool);
    
private:
    void executeCommand(const std::function<void(VkCommandBuffer)> &recorder, VkQueue queue, VkCommandPool cmdPool);

    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool keepChoices);
    void createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers);
    void createCommandPools();
    void createMainCommandPool(const VkCommandPoolCreateFlags flags, QueueContext& queueCtx);
    void createTextureSampler();
};