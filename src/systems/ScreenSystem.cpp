#include "systems/ScreenSystem.hpp"

#include "rendering/Camera.hpp"
#include "systems/VulkanSystem.hpp"
#include "time/Clock.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

ScreenSystem::ScreenSystem(WindowConfig config)
    : System("Screen"), _config(std::move(config)) {}

ScreenSystem::~ScreenSystem() {
    _renderer.reset();
    if (_statsWindow != nullptr) {
        glfwDestroyWindow(_statsWindow);
    }
    if (_window != nullptr) {
        glfwDestroyWindow(_window);
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
    _window = glfwCreateWindow(
        _config.width,
        _config.height,
        _config.title.c_str(),
        nullptr,
        nullptr
    );
    if (_window == nullptr) {
        throw std::runtime_error("Failed to create the EVO window");
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _statsWindow = glfwCreateWindow(
        360,
        120,
        "EVO Stats - Day 1",
        nullptr,
        nullptr
    );
    if (_statsWindow == nullptr) {
        throw std::runtime_error("Failed to create the EVO stats window");
    }

    glfwSetWindowUserPointer(_window, this);
    glfwSetKeyCallback(_window, _handleKey);
    glfwSetWindowCloseCallback(_window, _handleWindowClose);
    glfwSetCursorPosCallback(_window, _handleCursor);
    glfwSetMouseButtonCallback(_window, _handleMouseButton);
    glfwSetScrollCallback(_window, _handleScroll);
    glfwSetFramebufferSizeCallback(_window, _handleFramebufferSize);

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

    _renderer = std::make_unique<VulkanSystem>(_window, std::move(extensions));
    _renderer->init();
    System::init();
}

void ScreenSystem::setEventCallback(EventCallback callback) {
    _eventCallback = std::move(callback);
}

void ScreenSystem::pollEvents() {
    glfwPollEvents();
}

void ScreenSystem::onEvent(const Event& event) {
    if (event.type == EventType::EscapePressed) {
        glfwSetWindowShouldClose(_window, GLFW_TRUE);
    }
}

void ScreenSystem::render(const Land& land, const Sun& sun) {
    if (_camera == nullptr) {
        throw std::runtime_error("ScreenSystem requires a registered camera");
    }
    _renderer->render({.land = &land, .sun = sun, .camera = _camera->frame()});
    _updateStatsWindow();
}

void ScreenSystem::registerCamera(Camera& camera) {
    _camera = &camera;
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    _camera->setViewport(width, height);
}

void ScreenSystem::registerClock(Clock& clock) {
    _clock = &clock;
    _updateStatsWindow();
}

bool ScreenSystem::shouldClose() const {
    return glfwWindowShouldClose(_window) == GLFW_TRUE;
}

void ScreenSystem::_handleKey(
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
        screen->_emit({EventType::EscapePressed});
    }
}

void ScreenSystem::_handleWindowClose(GLFWwindow* window) {
    auto* screen = static_cast<ScreenSystem*>(glfwGetWindowUserPointer(window));
    screen->_emit({EventType::WindowCloseRequested});
}

void ScreenSystem::_handleCursor(GLFWwindow* window, double x, double y) {
    auto* screen = static_cast<ScreenSystem*>(glfwGetWindowUserPointer(window));
    screen->_emit({.type = EventType::MouseMoved, .x = x, .y = y});
}

void ScreenSystem::_handleMouseButton(
    GLFWwindow* window,
    int button,
    int action,
    int /* modifiers */
) {
    auto* screen = static_cast<ScreenSystem*>(glfwGetWindowUserPointer(window));
    const EventType type = action == GLFW_PRESS
        ? EventType::MouseButtonPressed
        : EventType::MouseButtonReleased;
    screen->_emit({.type = type, .button = button});
}

void ScreenSystem::_handleScroll(GLFWwindow* window, double x, double y) {
    auto* screen = static_cast<ScreenSystem*>(glfwGetWindowUserPointer(window));
    screen->_emit({.type = EventType::MouseScrolled, .x = x, .y = y});
}

void ScreenSystem::_handleFramebufferSize(
    GLFWwindow* window,
    int width,
    int height
) {
    auto* screen = static_cast<ScreenSystem*>(glfwGetWindowUserPointer(window));
    screen->_emit({
        .type = EventType::ViewportResized,
        .x = static_cast<double>(width),
        .y = static_cast<double>(height),
    });
}

void ScreenSystem::_emit(Event event) const {
    if (_eventCallback) {
        _eventCallback(event);
    }
}

void ScreenSystem::_updateStatsWindow() {
    if (_clock == nullptr || _statsWindow == nullptr) {
        return;
    }
    const std::uint64_t day = _clock->frame().day;
    if (day == _displayedDay) {
        return;
    }
    _displayedDay = day;
    const std::string title = "EVO Stats - Day " + std::to_string(day);
    glfwSetWindowTitle(_statsWindow, title.c_str());
}
