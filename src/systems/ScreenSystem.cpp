#include "systems/ScreenSystem.hpp"

#include "systems/VulkanSystem.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

ScreenSystem::ScreenSystem(WindowConfig config)
    : System("Screen"), config_(std::move(config)) {}

ScreenSystem::~ScreenSystem() {
    renderer_.reset();
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

void ScreenSystem::init() {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    if (glfwVulkanSupported() != GLFW_TRUE) {
        throw std::runtime_error("No Vulkan loader or compatible driver was found");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(
        config_.width,
        config_.height,
        config_.title.c_str(),
        nullptr,
        nullptr
    );
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create the EVO window");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, handleKey);
    glfwSetWindowCloseCallback(window_, handleWindowClose);

    std::uint32_t extensionCount = 0;
    const char** requiredExtensions =
        glfwGetRequiredInstanceExtensions(&extensionCount);
    if (requiredExtensions == nullptr || extensionCount == 0) {
        throw std::runtime_error("Failed to obtain Vulkan window extensions");
    }

    std::vector<std::string> extensions;
    extensions.reserve(extensionCount);
    for (std::uint32_t index = 0; index < extensionCount; ++index) {
        extensions.emplace_back(requiredExtensions[index]);
    }

    renderer_ = std::make_unique<VulkanSystem>(std::move(extensions));
    renderer_->init();
    System::init();
}

void ScreenSystem::setEventCallback(EventCallback callback) {
    eventCallback_ = std::move(callback);
}

void ScreenSystem::pollEvents() {
    glfwPollEvents();
}

void ScreenSystem::onEvent(const Event& event) {
    if (event.type == EventType::EscapePressed) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void ScreenSystem::render() {
    renderer_->render();
}

bool ScreenSystem::shouldClose() const {
    return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

void ScreenSystem::handleKey(
    GLFWwindow* window,
    int key,
    int /* scanCode */,
    int action,
    int /* modifiers */
) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        auto* screen = static_cast<ScreenSystem*>(
            glfwGetWindowUserPointer(window)
        );
        screen->emit({EventType::EscapePressed});
    }
}

void ScreenSystem::handleWindowClose(GLFWwindow* window) {
    auto* screen = static_cast<ScreenSystem*>(glfwGetWindowUserPointer(window));
    screen->emit({EventType::WindowCloseRequested});
}

void ScreenSystem::emit(Event event) const {
    if (eventCallback_) {
        eventCallback_(event);
    }
}
