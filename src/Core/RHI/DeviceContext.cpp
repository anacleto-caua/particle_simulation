#include "DeviceContext.hpp"

#include <cstdint>
#include <set>
#include <map>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "Core/RHI/Types/AppTypes.hpp"
#include "Core/RHI/Types/QueueCriteria.hpp"

DeviceContext::DeviceContext(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*> requiredDeviceExtensions, bool enableValidationLayers, std::vector<const char *> validationLayers) {
    m_requiredDeviceExtensions = requiredDeviceExtensions;
    pickPhysicalDevice(instance, surface);
    
    if (!findQueueFamilies(m_physicalDevice, surface, true)) {
        throw std::runtime_error("Failed to find valid queues during initialization!");
    }

    std::cout << "Picked queues -v-\n";
    std::cout << "Graphics queue index: " << m_graphicsQueueCtx.queueFamilyIndex << "\n";
    std::cout << "Transfer queue index: " << m_transferQueueCtx.queueFamilyIndex << "\n";
    std::cout << "Present queue index: " << m_presentQueueCtx.queueFamilyIndex << "\n";
    std::cout << "Compute queue index: " << m_computeQueueCtx.queueFamilyIndex << "\n";

    createLogicalDevice(surface, enableValidationLayers, validationLayers);
    createCommandPools();
    createTextureSampler();
}

DeviceContext::~DeviceContext() {
    vkDestroyCommandPool(m_logicalDevice, m_graphicsQueueCtx.mainCmdPool, nullptr);
    vkDestroyCommandPool(m_logicalDevice, m_transferQueueCtx.mainCmdPool, nullptr);
    vkDestroyCommandPool(m_logicalDevice, m_computeQueueCtx.mainCmdPool, nullptr);

    vkDestroySampler(m_logicalDevice, m_textureSampler, nullptr);
    
    if(m_logicalDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(m_logicalDevice, nullptr);
    }
}

void DeviceContext::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount < 1) {
        throw std::runtime_error("failed to find any devices!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

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

    m_physicalDevice = candidates.rbegin()->second;

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
    bool areIndicesEnough = findQueueFamilies(device, surface, false);

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
            if(std::strcmp(requiredExtension, availableExtension.extensionName) == 0){
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

bool DeviceContext::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool keepChoices) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    QueueCriteria baseCriteria =
        QueueCriteria::startCriteria(nullptr)
            .desireExclusivenessAgainst(&m_presentQueueCtx)
            .desireExclusivenessAgainst(&m_graphicsQueueCtx)
            .desireExclusivenessAgainst(&m_transferQueueCtx)
            .desireExclusivenessAgainst(&m_computeQueueCtx);
    
    QueueCriteria presentCriteria =
        QueueCriteria::startCriteria(baseCriteria, &m_presentQueueCtx)
            .requireSurfaceSupport(m_physicalDevice, surface);
    
    QueueCriteria graphicsCriteria =
        QueueCriteria::startCriteria(baseCriteria, &m_graphicsQueueCtx)
            .addRequiredFlags(VK_QUEUE_GRAPHICS_BIT);

    QueueCriteria transferCriteria =
        QueueCriteria::startCriteria(baseCriteria, &m_transferQueueCtx)
            .addRequiredFlags(VK_QUEUE_TRANSFER_BIT)
            .addAvoidedFlags(VK_QUEUE_GRAPHICS_BIT)
            .addAvoidedFlags(VK_QUEUE_COMPUTE_BIT);

    QueueCriteria computeCriteria =
        QueueCriteria::startCriteria(baseCriteria, &m_computeQueueCtx)
            .addRequiredFlags(VK_QUEUE_COMPUTE_BIT)
            .addAvoidedFlags(VK_QUEUE_GRAPHICS_BIT)
            .addAvoidedFlags(VK_QUEUE_TRANSFER_BIT);

    std::vector<QueueCriteria*> criterias = {
        &presentCriteria,
        &graphicsCriteria,
        &transferCriteria,
        &computeCriteria
    };

    for(QueueCriteria* criteria : criterias) {
        int32_t bestIndex = criteria->evaluateQueues(queueFamilies);

        if(bestIndex == -1) {
            return false;
        }
        
        if(keepChoices) {
            criteria->m_queueCtxToFit->queueFamilyIndex = static_cast<uint32_t>(bestIndex);
        }
    }
    
    return true;
}

void DeviceContext::createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_graphicsQueueCtx.queueFamilyIndex,
        m_presentQueueCtx.queueFamilyIndex,
        m_transferQueueCtx.queueFamilyIndex,
        m_computeQueueCtx.queueFamilyIndex
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

    std::cout << "Enabling " << m_requiredDeviceExtensions.size() << " extensions." << std::endl;
    for(const auto* name : m_requiredDeviceExtensions) {
        std::cout << " - " << name << std::endl;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueCtx.queueFamilyIndex, 0, &m_graphicsQueueCtx.queue);
    vkGetDeviceQueue(m_logicalDevice, m_transferQueueCtx.queueFamilyIndex, 0, &m_transferQueueCtx.queue);
    vkGetDeviceQueue(m_logicalDevice, m_presentQueueCtx.queueFamilyIndex, 0, &m_presentQueueCtx.queue);
    vkGetDeviceQueue(m_logicalDevice, m_computeQueueCtx.queueFamilyIndex, 0, &m_computeQueueCtx.queue);
}

void DeviceContext::createCommandPools() {
    createMainCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_graphicsQueueCtx);
    createMainCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_transferQueueCtx);
    createMainCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_computeQueueCtx);
}

void DeviceContext::createMainCommandPool(const VkCommandPoolCreateFlags flags, QueueContext& queueCtx) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueCtx.queueFamilyIndex;

    if (vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &queueCtx.mainCmdPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create main command pool!");
    }
}

void DeviceContext::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;   // About over sampling
    samplerInfo.minFilter = VK_FILTER_LINEAR;   // About under sampling

    // There are many modes, the repeat may be the most common because 
    // it let's you do repeat stuff like tile floors and tile walls
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    
    // Quering the m_logicalDevice properties so we know what anisotropy we can use
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // Just uses the max since performance isn't a concern

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Color to sample for outside of the texture, no arbitrary color, just black, white or transparent
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // Lod related
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    //samplerInfo.minLod = static_cast<float>(mipLevels / executeCommand2); // <---- uncomment to test the LODs
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(m_logicalDevice, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void DeviceContext::executeCommand(const std::function<void(VkCommandBuffer)> &recorder, const QueueContext &queueCtx) {
    executeCommand(recorder, queueCtx.queue, queueCtx.mainCmdPool);
}

void DeviceContext::executeCommand(const std::function<void(VkCommandBuffer)> &recorder, const QueueContext &queueCtx, VkCommandPool cmdPool) {
    executeCommand(recorder, queueCtx.queue, cmdPool);
}

void DeviceContext::executeCommand(const std::function<void(VkCommandBuffer)> &recorder, VkQueue queue, VkCommandPool cmdPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer{};
    vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    // Run the lambda function provided by the caller :>
    recorder(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_logicalDevice, cmdPool, 1, &commandBuffer);
}

uint32_t DeviceContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) 
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find a suitable memory type!");
}

VkSampleCountFlagBits DeviceContext::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}