#pragma  once

#include <vector>

#include <vulkan/vulkan.h>

class WindowContext {
public:
    virtual ~WindowContext() = default;
    
    virtual std::vector<const char*> getRequiredExtensions() = 0;
    
    virtual void createSurface(VkInstance instance, VkSurfaceKHR &surface) = 0;

    virtual void getFramebufferSize(uint32_t &width, uint32_t &height) = 0;

    virtual void waitEvents() = 0;

    virtual void update() = 0;

    virtual bool shouldClose() = 0;

};