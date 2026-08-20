#pragma once

#include "systems/RenderSystem.hpp"

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <vector>

struct GLFWwindow;

class VulkanSystem final : public RenderSystem {
public:
    VulkanSystem(
        GLFWwindow* window,
        std::vector<std::string> requiredExtensions
    );
    ~VulkanSystem() override;

    void init() override;
    void render(const Scene& scene) override;

private:
    struct Buffer {
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    void _createSwapchain();
    void _destroySwapchain();
    void _createPipeline();
    void _createShadowResources();
    void _createShadowDescriptors();
    void _destroyShadowResources();
    void _createCommands();
    void _createSyncObjects();
    void _recreateSwapchain();
    void _uploadLand(const Land& land);
    void _uploadSun();
    [[nodiscard]] Buffer _createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage
    ) const;
    [[nodiscard]] std::uint32_t _findMemoryType(
        std::uint32_t allowedTypes,
        VkMemoryPropertyFlags properties
    ) const;
    [[nodiscard]] VkShaderModule _loadShader(const std::string& path) const;
    [[nodiscard]] glm::mat4 _lightViewProjection(const Sun& sun) const;

    GLFWwindow* _window;
    std::vector<std::string> _requiredExtensions;
    VkInstance _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    std::uint32_t _graphicsQueueFamily = 0;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkFormat _swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D _swapchainExtent{};
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainViews;
    VkFormat _depthFormat = VK_FORMAT_D32_SFLOAT;
    VkImage _depthImage = VK_NULL_HANDLE;
    VkDeviceMemory _depthMemory = VK_NULL_HANDLE;
    VkImageView _depthView = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    static constexpr std::uint32_t shadowMapSize = 2048;
    VkImage _shadowImage = VK_NULL_HANDLE;
    VkDeviceMemory _shadowMemory = VK_NULL_HANDLE;
    VkImageView _shadowView = VK_NULL_HANDLE;
    VkSampler _shadowSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout _shadowDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool _shadowDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet _shadowDescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout _shadowPipelineLayout = VK_NULL_HANDLE;
    VkPipeline _shadowPipeline = VK_NULL_HANDLE;
    Buffer _shadowUniformBuffer;
    bool _shadowMapInitialized = false;
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    VkSemaphore _imageAvailable = VK_NULL_HANDLE;
    VkSemaphore _renderFinished = VK_NULL_HANDLE;
    VkFence _frameFence = VK_NULL_HANDLE;
    Buffer _vertexBuffer;
    Buffer _indexBuffer;
    std::uint32_t _indexCount = 0;
    std::uint64_t _landRevision = 0;
    glm::vec3 _landCenter{0.0F};
    float _landRadius = 1.0F;
    Buffer _sunVertexBuffer;
    Buffer _sunIndexBuffer;
    std::uint32_t _sunIndexCount = 0;
};
