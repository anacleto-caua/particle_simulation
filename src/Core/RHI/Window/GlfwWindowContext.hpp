#pragma  once

#include "WindowContext.hpp"

#include <vector>
#include <string>
#include <functional>

#include <GLFW/glfw3.h>

class GlfwWindowContext: public WindowContext {
public:
    using ResizeCallback = std::function<void(int width, int height)>;

    GLFWwindow *m_glfwWindow = nullptr;

    GlfwWindowContext(uint32_t width, uint32_t height, const std::string &title, ResizeCallback callback);
    ~GlfwWindowContext() override;

    std::vector<const char*> getRequiredExtensions() override;

    void createSurface(VkInstance instance, VkSurfaceKHR &surface) override;

    void getFramebufferSize(uint32_t &width, uint32_t &height) override;

    void waitEvents() override;

    void update() override;
    
    bool shouldClose() override;

    double getTime() override;

private:
    ResizeCallback m_userResizeCallback;

    static void staticFramebufferResizeCallback(GLFWwindow* window, int width, int height);
    void onResize(int width, int height);
};