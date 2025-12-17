#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "Types/AppTypes.hpp"

// Forward declare to avoid circular includes
struct VulkanContext; 

class DeviceContext {
public:
    DeviceContext(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> requiredDeviceExtensions, bool enableValidationLayers, std::vector<const char *> validationLayers);

    ~DeviceContext();

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;
    
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_logicalDevice = VK_NULL_HANDLE;

    //TODO: Maybe the transfer queue should be managed only under this class OR
    //  maybe the queues shoulbe be managed inside this class implicitilly
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkQueue m_transferQueue;
    
    VkCommandPool m_graphicsCmdPool;
    VkCommandPool m_transferCmdPool;
    
    QueueFamilyIndices m_queueIndices;

    std::vector<const char*> m_requiredDeviceExtensions;
    
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface);
    // TOFIX: Somehow I doubt this is right
    void executeCommand(std::function<void(VkCommandBuffer)> recorder, VkCommandPool cmdPool, VkQueue queue);

private:
    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    void createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers);
    void createCommandPools();
};