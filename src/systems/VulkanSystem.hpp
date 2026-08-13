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
    void render() override;

private:
    std::vector<std::string> requiredExtensions_;
    VkInstance instance_ = VK_NULL_HANDLE;
};
