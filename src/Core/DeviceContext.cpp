#include "DeviceContext.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

DeviceContext::DeviceContext(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> requiredDeviceExtensions, bool enableValidationLayers, std::vector<const char *> validationLayers) {
    m_requiredDeviceExtensions = requiredDeviceExtensions;
    pickPhysicalDevice(instance, surface);
    m_queueIndices = findQueueFamilies(m_physicalDevice, surface);
    createLogicalDevice(surface, enableValidationLayers, validationLayers);
}

DeviceContext::~DeviceContext() {
    if(m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }
}

void DeviceContext::executeCommand(std::function<void(VkCommandBuffer)> recorder) {
    throw std::runtime_error("not yet implemented funcionality! - DeviceContext::executeCommand(std::function<void(VkCommandBuffer)> recorder)");
}

void DeviceContext::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    if (deviceCount < 1) {
        throw std::runtime_error("failed to find any devices!");
    }

    for (const auto &device : devices) {
        if (!isDeviceSuitable(device, surface)) {
            continue;
        }
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.empty()) {
        throw std::runtime_error("failed to find a suitable GPU! - empty candidates");
    }

    if (candidates.rbegin()->first > 0) {
        m_physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    // Chosen device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);

    std::cout << "Chosen Device: "          << deviceProperties.deviceName      << "\n";
    std::cout << "Device ID: "              << deviceProperties.deviceID        << "\n";
    std::cout << "Device Type: "            << deviceProperties.deviceType      << "\n";
    std::cout << "Device Driver Version: "  << deviceProperties.driverVersion   << "\n";
    std::cout << "Device Api Version: "     << deviceProperties.apiVersion      << "\n";
}

int DeviceContext::rateDeviceSuitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Preference for discrete GPUs
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

bool DeviceContext::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    bool areIndicesEnough = findQueueFamilies(device, surface).isComplete();

    bool areExtensionsSupported = checkDeviceExtensionSupport(device);

    bool isSwapChainAdequate = false;
    if (areExtensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        isSwapChainAdequate = !swapChainSupport.formats.empty() &&!swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    bool areFeaturesSupported = supportedFeatures.samplerAnisotropy && supportedFeatures.geometryShader;

    return areIndicesEnough && areExtensionsSupported && isSwapChainAdequate && areFeaturesSupported;
}

bool DeviceContext::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,availableExtensions.data());

    for(const auto &requiredExtension: m_requiredDeviceExtensions) {
        bool isExtAvailable = false;
        
        for (const auto &availableExtension : availableExtensions) {
            if(std::strcmp(requiredExtension, availableExtension.extensionName)){
                isExtAvailable = true;
                break;
            }
        }

        if(!isExtAvailable) {
            return false;
        }
    }

    return true;
}

SwapChainSupportDetails DeviceContext::querySwapChainSupport(VkSurfaceKHR surface) {
    return querySwapChainSupport(m_physicalDevice, surface);
}

SwapChainSupportDetails DeviceContext::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void DeviceContext::createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_queueIndices.graphicsFamily.value(),
        m_queueIndices.presentFamily.value(),
        m_queueIndices.transferFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Device specific features we wanna use
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;
    
    VkPhysicalDeviceSynchronization2Features sync2Features = {};
    sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = VK_TRUE;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_requiredDeviceExtensions.data();
    createInfo.pNext = &sync2Features;

    if (enableValidationLayers) {
        // Both parameters are not used anymore but it's recommended to set for backwards compatibility
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_device, m_queueIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_queueIndices.transferFamily.value(), 0, &m_transferQueue);
    vkGetDeviceQueue(m_device, m_queueIndices.presentFamily.value(), 0, &m_presentQueue);
}

QueueFamilyIndices DeviceContext::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
        nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
        queueFamilies.data());

    int i = 0;
    VkBool32 presentSupport = false;
    for (const auto& queueFamily : queueFamilies) {

        // Does the queue support surface presentation
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        // If the queue is suitable for surface presentation
        if (!(indices.presentFamily.has_value()) &&
            (presentSupport)
            ) {
            indices.presentFamily = i;
        }

        // Picks a queue family with both graphics and compute capabilities
        if (!(indices.graphicsFamily.has_value()) &&
            (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            ) {
            indices.graphicsFamily = i;
        }

        // Picks a transfer exclusive queue
        if (!(indices.transferFamily.has_value()) &&
            (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            ) {
            indices.transferFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    // If no dedicated transfer family was found pick the next best thing 
    // loops from back to front as trying not to pick the same queues for present and graphics
    if (!indices.transferFamily.has_value()) {
        for (i = queueFamilies.size() - 1; i >= 0; i--) {
            auto& queueFamily = queueFamilies.at(i);

            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) ||
                (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
                ) {
                indices.transferFamily = i;
                break;
            }
        }
    }

    if (!indices.isComplete()) {
        throw std::runtime_error("couldn't find the necessary queue families");
    }

    return indices;
}
