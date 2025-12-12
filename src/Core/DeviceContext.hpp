#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "Types/AppTypes.hpp"

// Forward declare to avoid circular includes
struct VulkanContext; 

class DeviceContext {
public:
    DeviceContext(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> requiredDeviceExtensions, bool enableValidationLayers, std::vector<const char *> validationLayers);
    
    ~DeviceContext();

    DeviceContext(const DeviceContext&) = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;

    VkDevice getLogicalDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkQueue getTransferQueue() const { return m_transferQueue; }    //TODO: Maybe the transfer queue should be managed only under this class
    QueueFamilyIndices getQueueFamilyIndices() const { return m_queueIndices; }
    
    void executeCommand(std::function<void(VkCommandBuffer)> recorder);
    SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface);

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkQueue m_transferQueue;

    QueueFamilyIndices m_queueIndices;

    std::vector<const char*> m_requiredDeviceExtensions;

    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    void createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers);
    
};