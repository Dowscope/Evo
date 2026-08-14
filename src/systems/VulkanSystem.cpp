#include "systems/VulkanSystem.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

VulkanSystem::VulkanSystem(std::vector<std::string> requiredExtensions)
    : RenderSystem("Vulkan"),
      _requiredExtensions(std::move(requiredExtensions)) {}

VulkanSystem::~VulkanSystem() {
    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
    }
}

void VulkanSystem::init() {
    std::vector<const char*> extensions;
    extensions.reserve(_requiredExtensions.size());
    for (const std::string& extension : _requiredExtensions) {
        extensions.push_back(extension.c_str());
    }

    const VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "EVO",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = "EVO",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    const VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan instance");
    }

    System::init();
}

void VulkanSystem::render(const Scene& /* scene */) {
    // Rendering commands, swapchain presentation, and Vulkan synchronization
    // remain encapsulated here as the renderer grows.
}
