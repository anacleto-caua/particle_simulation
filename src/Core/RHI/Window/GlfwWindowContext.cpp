#include "GlfwWindowContext.hpp"
#include <cstdint>
#include <stdexcept>

GlfwWindowContext::GlfwWindowContext(uint32_t width, uint32_t height, const std::string &title, ResizeCallback callback) : m_userResizeCallback(callback) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_glfwWindow, this);
    glfwSetFramebufferSizeCallback(m_glfwWindow, staticFramebufferResizeCallback);
}

GlfwWindowContext::~GlfwWindowContext() {
    glfwDestroyWindow(m_glfwWindow);
    glfwTerminate();
}

std::vector<const char*> GlfwWindowContext::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<const char*> (glfwExtensions, glfwExtensions + glfwExtensionCount);
}

void GlfwWindowContext::createSurface(VkInstance instance, VkSurfaceKHR &surface) {
    if (glfwCreateWindowSurface(instance, m_glfwWindow, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void GlfwWindowContext::waitEvents() {
    glfwWaitEvents();
}

bool GlfwWindowContext::shouldClose() {
    return glfwWindowShouldClose(m_glfwWindow);
}

void GlfwWindowContext::update() {
    glfwPollEvents();
}

void GlfwWindowContext::getFramebufferSize(uint32_t &width, uint32_t &height) {
    int c_width = 0, c_height = 0;
    glfwGetFramebufferSize(m_glfwWindow, &c_width, &c_height);
    width = static_cast<uint32_t>(c_width);
    height = static_cast<uint32_t>(c_height);
}

void GlfwWindowContext::staticFramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    GlfwWindowContext* self = reinterpret_cast<GlfwWindowContext*>(glfwGetWindowUserPointer(window));
    self->onResize(width, height);
}

void GlfwWindowContext::onResize(int width, int height) {
    m_userResizeCallback(width, height);
}