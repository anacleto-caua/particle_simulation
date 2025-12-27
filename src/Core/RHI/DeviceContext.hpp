#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include <vulkan/vulkan_core.h>

#include "Types/AppTypes.hpp"

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
    
    // TODO: Make a proper migration from QueueFamilyIndices with the sparse VkCommandPool's and VkQueue's to use a single struct or object 
    QueueContext m_graphicsQueueCtx;
    QueueContext m_transferQueueCtx;
    QueueContext m_presentQueueCtx;

    std::vector<const char*> m_requiredDeviceExtensions;
    
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // TOFIX: Somehow I doubt this is right
    void executeCommand(const std::function<void(VkCommandBuffer)> &recorder, VkCommandPool cmdPool, VkQueue queue);
    // TODO: That's a temporary overwrite before a proper migration
    void executeCommand(const std::function<void(VkCommandBuffer)> &recorder, const QueueContext &queueCtx);

private:
    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool keepChoices);
    void createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers);
    void createCommandPools();
    void createTextureSampler();
};