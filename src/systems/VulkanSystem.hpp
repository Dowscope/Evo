#pragma once

#include "systems/RenderSystem.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class VulkanSystem final : public RenderSystem {
public:
    explicit VulkanSystem(std::vector<std::string> requiredExtensions);
    ~VulkanSystem() override;

    void init() override;
    void render(const Scene& scene) override;

private:
    std::vector<std::string> _requiredExtensions;
    VkInstance _instance = VK_NULL_HANDLE;
};
