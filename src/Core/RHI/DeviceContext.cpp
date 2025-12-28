#include "DeviceContext.hpp"

#include <set>
#include <map>
#include <cstring>
#include <iostream>
#include <stdexcept>

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

    createLogicalDevice(surface, enableValidationLayers, validationLayers);
    createCommandPools();
    createTextureSampler();
}

DeviceContext::~DeviceContext() {
    vkDestroyCommandPool(m_logicalDevice, m_graphicsQueueCtx.mainCmdPool, nullptr);
    vkDestroyCommandPool(m_logicalDevice, m_transferQueueCtx.mainCmdPool, nullptr);

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

    int32_t bestGraphicsQIndex = -1;
    int32_t bestTransferQIndex = -1;
    int32_t bestPresentQIndex = -1;

    QueueCriteria baseCriteria =
        QueueCriteria::startCriteria()
            .desireExclusivenessAgainst(m_presentQueueCtx)
            .desireExclusivenessAgainst(m_graphicsQueueCtx)
            .desireExclusivenessAgainst(m_presentQueueCtx);

    bestPresentQIndex = 
        QueueCriteria::startCriteria(baseCriteria)
            .requireSurfaceSupport(m_physicalDevice, surface)
            .evaluateQueues(queueFamilies);
    if(keepChoices) {
        m_presentQueueCtx.queueFamilyIndex = static_cast<uint32_t>(bestPresentQIndex);
    }

    bestGraphicsQIndex = 
        QueueCriteria::startCriteria(baseCriteria)
            .addRequiredFlags(VK_QUEUE_GRAPHICS_BIT)
            // .addRequiredFlags(VK_QUEUE_COMPUTE_BIT)
            .evaluateQueues(queueFamilies);
    if(keepChoices) {
        m_graphicsQueueCtx.queueFamilyIndex = static_cast<uint32_t>(bestGraphicsQIndex);
    }

    bestTransferQIndex = 
        QueueCriteria::startCriteria(baseCriteria)
            .addRequiredFlags(VK_QUEUE_TRANSFER_BIT)
            .addAvoidedFlags(VK_QUEUE_GRAPHICS_BIT)
            .addAvoidedFlags(VK_QUEUE_COMPUTE_BIT)
            .evaluateQueues(queueFamilies);
    if(keepChoices) {
        m_transferQueueCtx.queueFamilyIndex = static_cast<uint32_t>(bestTransferQIndex);
    }

    bool querySuccess = false;
    if(
        (
            bestGraphicsQIndex != -1 &&
            bestTransferQIndex != -1 &&
            bestPresentQIndex != -1
        ) 
    ) {
        querySuccess = true;
    }

    return querySuccess;
}

void DeviceContext::createLogicalDevice(VkSurfaceKHR surface, bool enableValidationLayers, std::vector<const char *> validationLayers) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_graphicsQueueCtx.queueFamilyIndex,
        m_presentQueueCtx.queueFamilyIndex,
        m_transferQueueCtx.queueFamilyIndex
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
}

void DeviceContext::createCommandPools() {
    VkCommandPoolCreateInfo poolInfoGraphics{};
    poolInfoGraphics.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfoGraphics.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfoGraphics.queueFamilyIndex = m_graphicsQueueCtx.queueFamilyIndex;

    if (vkCreateCommandPool(m_logicalDevice, &poolInfoGraphics, nullptr, &m_graphicsQueueCtx.mainCmdPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }

    VkCommandPoolCreateInfo poolInfoTransfer{};
    poolInfoTransfer.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfoTransfer.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfoTransfer.queueFamilyIndex = m_transferQueueCtx.queueFamilyIndex;

    if (vkCreateCommandPool(m_logicalDevice, &poolInfoTransfer, nullptr, &m_transferQueueCtx.mainCmdPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create transfer command pool!");
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
    //samplerInfo.minLod = static_cast<float>(mipLevels / 2); // <---- uncomment to test the LODs
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(m_logicalDevice, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void DeviceContext::executeCommand(const std::function<void(VkCommandBuffer)> &recorder, const QueueContext &queueCtx) {
    executeCommand(recorder, queueCtx.mainCmdPool, queueCtx.queue);
}

void DeviceContext::executeCommand(const std::function<void(VkCommandBuffer)> &recorder, VkCommandPool cmdPool, VkQueue queue) {
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