#include "systems/ScreenSystem.hpp"

#include "rendering/Camera.hpp"
#include "systems/VulkanSystem.hpp"
#include "simulation/SurfaceTemperatureStatistics.hpp"
#include "simulation/AtmosphereStatistics.hpp"
#include "time/Clock.hpp"

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::array<unsigned char, 7> glyph(char character) {
    switch (character) {
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'I': return {14, 4, 4, 4, 4, 4, 14};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case ':': return {0, 4, 4, 0, 4, 4, 0};
    case '.': return {0, 0, 0, 0, 0, 4, 4};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    default: return {};
    }
}

void drawText(const std::string& value, float x, float y, float scale) {
    glBegin(GL_QUADS);
    for (const char character : value) {
        const auto pixels = glyph(character);
        for (std::size_t row = 0; row < pixels.size(); ++row) {
            for (std::size_t column = 0; column < 5; ++column) {
                if ((pixels[row] & (1U << (4U - column))) == 0) {
                    continue;
                }
                const float left = x + static_cast<float>(column) * scale;
                const float top = y + static_cast<float>(row) * scale;
                glVertex2f(left, top);
                glVertex2f(left + scale, top);
                glVertex2f(left + scale, top + scale);
                glVertex2f(left, top + scale);
            }
        }
        x += scale * 6.0F;
    }
    glEnd();
}

} // namespace

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

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _statsWindow = glfwCreateWindow(
        420,
        250,
        "EVO Stats",
        nullptr,
        nullptr
    );
    if (_statsWindow == nullptr) {
        throw std::runtime_error("Failed to create the EVO stats window");
    }
    glfwSetWindowUserPointer(_statsWindow, this);
    glfwSetKeyCallback(_statsWindow, _handleKey);
    glfwMakeContextCurrent(_statsWindow);
    glfwSwapInterval(0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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
    } else if (event.type == EventType::TemperatureOverlayToggled) {
        _temperatureOverlay = !_temperatureOverlay;
    }
}

void ScreenSystem::render(const Land& land, const Sun& sun) {
    if (_camera == nullptr) {
        throw std::runtime_error("ScreenSystem requires a registered camera");
    }
    _renderer->render({
        .land = &land,
        .sun = sun,
        .camera = _camera->frame(),
        .temperatureOverlay = _temperatureOverlay,
    });
    _renderStatsWindow();
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
}

void ScreenSystem::registerTemperatureStatistics(
    SurfaceTemperatureStatistics& statistics
) {
    _temperatureStatistics = &statistics;
}

void ScreenSystem::registerAtmosphereStatistics(
    AtmosphereStatistics& statistics
) {
    _atmosphereStatistics = &statistics;
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
    } else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        auto* screen = static_cast<ScreenSystem*>(
            glfwGetWindowUserPointer(window)
        );
        screen->_emit({EventType::TemperatureOverlayToggled});
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

void ScreenSystem::_renderStatsWindow() {
    if (_clock == nullptr || _temperatureStatistics == nullptr ||
        _atmosphereStatistics == nullptr ||
        _statsWindow == nullptr ||
        glfwWindowShouldClose(_statsWindow) == GLFW_TRUE) {
        return;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_statsWindow, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }

    glfwMakeContextCurrent(_statsWindow);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height),
            0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.035F, 0.055F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    const std::uint64_t day = _clock->frame().day;
    const float averageTemperature =
        _temperatureStatistics->averageSurfaceTemperatureCelsius();
    const AtmosphereState& atmosphere =
        _atmosphereStatistics->atmosphere();
    std::ostringstream temperature;
    temperature << "AVG TEMP: " << std::fixed << std::setprecision(1)
                << averageTemperature << " C";
    std::ostringstream airTemperature;
    airTemperature << "AIR TEMP: " << std::fixed << std::setprecision(1)
                   << atmosphere.airTemperatureCelsius << " C";
    std::ostringstream skyTemperature;
    skyTemperature << "SKY TEMP: " << std::fixed << std::setprecision(1)
                   << atmosphere.effectiveSkyTemperatureCelsius << " C";

    glColor3f(0.35F, 0.78F, 1.0F);
    drawText("EVO STATS", 20.0F, 18.0F, 4.0F);
    glColor3f(0.88F, 0.93F, 1.0F);
    drawText("DAY: " + std::to_string(day), 20.0F, 62.0F, 3.0F);
    drawText(temperature.str(), 20.0F, 96.0F, 3.0F);
    drawText(airTemperature.str(), 20.0F, 130.0F, 3.0F);
    drawText(skyTemperature.str(), 20.0F, 164.0F, 3.0F);
    drawText(
        _temperatureOverlay ? "OVERLAY: ON" : "OVERLAY: OFF",
        20.0F,
        198.0F,
        3.0F
    );

    glfwSwapBuffers(_statsWindow);
}
