#include "systems/VulkanSystem.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef EVO_SHADER_DIR
#define EVO_SHADER_DIR "shaders"
#endif

namespace {
struct DrawState {
    glm::mat4 viewProjection;
    glm::vec4 modelTranslation;
    glm::vec4 sun;
    glm::vec4 display;
};

struct ShadowState {
    glm::mat4 lightViewProjection;
};

void require(VkResult result, const char* message) {
    if (result != VK_SUCCESS) throw std::runtime_error(message);
}
}

VulkanSystem::VulkanSystem(GLFWwindow* window, std::vector<std::string> extensions)
    : RenderSystem("Vulkan"), _window(window),
      _requiredExtensions(std::move(extensions)) {}

VulkanSystem::~VulkanSystem() {
    if (_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
        if (_vertexBuffer.handle) {
            vkDestroyBuffer(_device, _vertexBuffer.handle, nullptr);
            vkFreeMemory(_device, _vertexBuffer.memory, nullptr);
        }
        if (_indexBuffer.handle) {
            vkDestroyBuffer(_device, _indexBuffer.handle, nullptr);
            vkFreeMemory(_device, _indexBuffer.memory, nullptr);
        }
        if (_sunVertexBuffer.handle) {
            vkDestroyBuffer(_device, _sunVertexBuffer.handle, nullptr);
            vkFreeMemory(_device, _sunVertexBuffer.memory, nullptr);
            vkDestroyBuffer(_device, _sunIndexBuffer.handle, nullptr);
            vkFreeMemory(_device, _sunIndexBuffer.memory, nullptr);
        }
        if (_frameFence) {
            vkDestroyFence(_device, _frameFence, nullptr);
            vkDestroySemaphore(_device, _renderFinished, nullptr);
            vkDestroySemaphore(_device, _imageAvailable, nullptr);
        }
        if (_commandPool) vkDestroyCommandPool(_device, _commandPool, nullptr);
        _destroySwapchain();
        _destroyShadowResources();
        vkDestroyDevice(_device, nullptr);
    }
    if (_surface) vkDestroySurfaceKHR(_instance, _surface, nullptr);
    if (_instance) vkDestroyInstance(_instance, nullptr);
}

void VulkanSystem::init() {
    std::vector<const char*> extensions;
    for (const auto& extension : _requiredExtensions) extensions.push_back(extension.c_str());
    const VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "EVO",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0), .pEngineName = "EVO",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0), .apiVersion = VK_API_VERSION_1_3};
    const VkInstanceCreateInfo instanceInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()};
    require(vkCreateInstance(&instanceInfo, nullptr, &_instance), "Failed to create Vulkan instance");
    require(glfwCreateWindowSurface(_instance, _window, nullptr, &_surface),
            "Failed to create Vulkan surface");

    std::uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
    for (auto device : devices) {
        std::uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
        for (std::uint32_t index = 0; index < count; ++index) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, index, _surface, &present);
            if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                _physicalDevice = device;
                _graphicsQueueFamily = index;
                break;
            }
        }
        if (_physicalDevice) break;
    }
    if (!_physicalDevice) throw std::runtime_error("No Vulkan device can present to the window");
    constexpr float priority = 1.0F;
    const VkDeviceQueueCreateInfo queueInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = _graphicsQueueFamily, .queueCount = 1, .pQueuePriorities = &priority};
    constexpr std::array deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE};
    const VkDeviceCreateInfo deviceInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRendering, .queueCreateInfoCount = 1, .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()};
    require(vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device),
            "Failed to create Vulkan device");
    vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);
    _createShadowResources();
    _createShadowDescriptors();
    _createSwapchain();
    _createCommands();
    _createSyncObjects();
    System::init();
}

void VulkanSystem::_createShadowResources() {
    const VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = _depthFormat,
        .extent = {shadowMapSize, shadowMapSize, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    require(
        vkCreateImage(_device, &imageInfo, nullptr, &_shadowImage),
        "Failed to create shadow image"
    );
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(_device, _shadowImage, &requirements);
    const VkMemoryAllocateInfo allocation{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = _findMemoryType(
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        ),
    };
    require(
        vkAllocateMemory(_device, &allocation, nullptr, &_shadowMemory),
        "Failed to allocate shadow memory"
    );
    require(
        vkBindImageMemory(_device, _shadowImage, _shadowMemory, 0),
        "Failed to bind shadow memory"
    );
    const VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = _shadowImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = _depthFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    require(
        vkCreateImageView(_device, &viewInfo, nullptr, &_shadowView),
        "Failed to create shadow view"
    );
    const VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    };
    require(
        vkCreateSampler(_device, &samplerInfo, nullptr, &_shadowSampler),
        "Failed to create shadow sampler"
    );
    _shadowUniformBuffer = _createBuffer(
        sizeof(ShadowState),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
}

void VulkanSystem::_createShadowDescriptors() {
    const std::array bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
    };
    const VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    require(
        vkCreateDescriptorSetLayout(
            _device, &layoutInfo, nullptr, &_shadowDescriptorLayout
        ),
        "Failed to create shadow descriptor layout"
    );
    const std::array poolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
    };
    const VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = static_cast<std::uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    require(
        vkCreateDescriptorPool(
            _device, &poolInfo, nullptr, &_shadowDescriptorPool
        ),
        "Failed to create shadow descriptor pool"
    );
    const VkDescriptorSetAllocateInfo setInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = _shadowDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &_shadowDescriptorLayout,
    };
    require(
        vkAllocateDescriptorSets(_device, &setInfo, &_shadowDescriptorSet),
        "Failed to allocate shadow descriptor set"
    );
    const VkDescriptorImageInfo image{
        .sampler = _shadowSampler,
        .imageView = _shadowView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    const VkDescriptorBufferInfo buffer{
        .buffer = _shadowUniformBuffer.handle,
        .offset = 0,
        .range = sizeof(ShadowState),
    };
    const std::array writes{
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = _shadowDescriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = _shadowDescriptorSet,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer,
        },
    };
    vkUpdateDescriptorSets(
        _device,
        static_cast<std::uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}

void VulkanSystem::_createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);
    std::uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &count, formats.data());
    if (formats.empty()) throw std::runtime_error("Vulkan surface has no formats");
    auto selected = formats.front();
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) selected = format;
    }
    _swapchainFormat = selected.format;
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        _swapchainExtent = capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);
        _swapchainExtent = {
            std::clamp(static_cast<std::uint32_t>(width), capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width),
            std::clamp(static_cast<std::uint32_t>(height), capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height)};
    }
    std::uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount) imageCount = std::min(imageCount, capabilities.maxImageCount);
    const VkSwapchainCreateInfoKHR info{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = _surface, .minImageCount = imageCount, .imageFormat = _swapchainFormat,
        .imageColorSpace = selected.colorSpace, .imageExtent = _swapchainExtent,
        .imageArrayLayers = 1, .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR, .clipped = VK_TRUE};
    require(vkCreateSwapchainKHR(_device, &info, nullptr, &_swapchain),
            "Failed to create Vulkan swapchain");
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
    _swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapchainImages.data());
    _swapchainViews.resize(imageCount);
    for (std::size_t index = 0; index < imageCount; ++index) {
        const VkImageViewCreateInfo viewInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = _swapchainImages[index], .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = _swapchainFormat, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
        require(vkCreateImageView(_device, &viewInfo, nullptr, &_swapchainViews[index]),
                "Failed to create swapchain view");
    }
    const VkImageCreateInfo depthInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D, .format = _depthFormat,
        .extent = {_swapchainExtent.width, _swapchainExtent.height, 1}, .mipLevels = 1,
        .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    require(vkCreateImage(_device, &depthInfo, nullptr, &_depthImage), "Failed to create depth image");
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(_device, _depthImage, &requirements);
    const VkMemoryAllocateInfo allocation{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = _findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    require(vkAllocateMemory(_device, &allocation, nullptr, &_depthMemory), "Failed to allocate depth memory");
    require(vkBindImageMemory(_device, _depthImage, _depthMemory, 0), "Failed to bind depth memory");
    const VkImageViewCreateInfo depthViewInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = _depthImage, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = _depthFormat,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMipLevel = 0,
            .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
    require(vkCreateImageView(_device, &depthViewInfo, nullptr, &_depthView),
            "Failed to create depth view");
    _createPipeline();
}

void VulkanSystem::_createPipeline() {
    auto vertexShader = _loadShader(std::string(EVO_SHADER_DIR) + "/land.vert.spv");
    auto fragmentShader = _loadShader(std::string(EVO_SHADER_DIR) + "/land.frag.spv");
    const std::array stages{
        VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vertexShader, .pName = "main"},
        VkPipelineShaderStageCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fragmentShader, .pName = "main"}};
    const VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    const std::array attributes{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT,
            static_cast<std::uint32_t>(offsetof(Vertex, position))},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT,
            static_cast<std::uint32_t>(offsetof(Vertex, color))},
        VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32_SFLOAT,
            static_cast<std::uint32_t>(offsetof(Vertex, surfaceTemperatureCelsius))}};
    const VkPipelineVertexInputStateCreateInfo vertexInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &binding,
        .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data()};
    const VkPipelineInputAssemblyStateCreateInfo assembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    const VkPipelineViewportStateCreateInfo viewport{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, .scissorCount = 1};
    const VkPipelineRasterizationStateCreateInfo rasterization{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1.0F};
    const VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
    const VkPipelineDepthStencilStateCreateInfo depth{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE, .depthWriteEnable = VK_TRUE, .depthCompareOp = VK_COMPARE_OP_LESS};
    const VkPipelineColorBlendAttachmentState blendAttachment{.colorWriteMask = 0xf};
    const VkPipelineColorBlendStateCreateInfo blending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1, .pAttachments = &blendAttachment};
    constexpr std::array dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    const VkPipelineDynamicStateCreateInfo dynamic{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};
    constexpr VkShaderStageFlags drawStages =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    const VkPushConstantRange pushConstant{.stageFlags = drawStages,
        .offset = 0, .size = sizeof(DrawState)};
    const VkPipelineLayoutCreateInfo layoutInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1, .pSetLayouts = &_shadowDescriptorLayout,
        .pushConstantRangeCount = 1, .pPushConstantRanges = &pushConstant};
    require(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_pipelineLayout),
            "Failed to create pipeline layout");
    const VkPipelineRenderingCreateInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1, .pColorAttachmentFormats = &_swapchainFormat,
        .depthAttachmentFormat = _depthFormat};
    const VkGraphicsPipelineCreateInfo pipelineInfo{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingInfo, .stageCount = static_cast<std::uint32_t>(stages.size()),
        .pStages = stages.data(), .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &assembly, .pViewportState = &viewport,
        .pRasterizationState = &rasterization, .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth, .pColorBlendState = &blending,
        .pDynamicState = &dynamic, .layout = _pipelineLayout};
    VkResult result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
    vkDestroyShaderModule(_device, fragmentShader, nullptr);
    vkDestroyShaderModule(_device, vertexShader, nullptr);
    require(result, "Failed to create graphics pipeline");

    auto shadowVertexShader = _loadShader(
        std::string(EVO_SHADER_DIR) + "/shadow.vert.spv"
    );
    const VkPipelineShaderStageCreateInfo shadowStage{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shadowVertexShader,
        .pName = "main",
    };
    const VkPushConstantRange shadowPushConstant{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(DrawState),
    };
    const VkPipelineLayoutCreateInfo shadowLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &shadowPushConstant,
    };
    require(
        vkCreatePipelineLayout(
            _device, &shadowLayoutInfo, nullptr, &_shadowPipelineLayout
        ),
        "Failed to create shadow pipeline layout"
    );
    const VkPipelineRasterizationStateCreateInfo shadowRasterization{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_TRUE,
        .depthBiasConstantFactor = 1.25F,
        .depthBiasClamp = 0.0F,
        .depthBiasSlopeFactor = 1.75F,
        .lineWidth = 1.0F,
    };
    const VkPipelineColorBlendStateCreateInfo shadowBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    };
    const VkPipelineRenderingCreateInfo shadowRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .depthAttachmentFormat = _depthFormat,
    };
    const VkGraphicsPipelineCreateInfo shadowPipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &shadowRenderingInfo,
        .stageCount = 1,
        .pStages = &shadowStage,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &assembly,
        .pViewportState = &viewport,
        .pRasterizationState = &shadowRasterization,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth,
        .pColorBlendState = &shadowBlending,
        .pDynamicState = &dynamic,
        .layout = _shadowPipelineLayout,
    };
    result = vkCreateGraphicsPipelines(
        _device,
        VK_NULL_HANDLE,
        1,
        &shadowPipelineInfo,
        nullptr,
        &_shadowPipeline
    );
    vkDestroyShaderModule(_device, shadowVertexShader, nullptr);
    require(result, "Failed to create shadow pipeline");
}

void VulkanSystem::_createCommands() {
    const VkCommandPoolCreateInfo info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _graphicsQueueFamily};
    require(vkCreateCommandPool(_device, &info, nullptr, &_commandPool), "Failed to create command pool");
    const VkCommandBufferAllocateInfo allocation{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _commandPool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
    require(vkAllocateCommandBuffers(_device, &allocation, &_commandBuffer), "Failed to allocate command buffer");
}

void VulkanSystem::_createSyncObjects() {
    const VkSemaphoreCreateInfo semaphore{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    const VkFenceCreateInfo fence{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    require(vkCreateSemaphore(_device, &semaphore, nullptr, &_imageAvailable), "Failed to create semaphore");
    require(vkCreateSemaphore(_device, &semaphore, nullptr, &_renderFinished), "Failed to create semaphore");
    require(vkCreateFence(_device, &fence, nullptr, &_frameFence), "Failed to create fence");
}

void VulkanSystem::render(const Scene& scene) {
    if (!scene.land || scene.land->indices.empty()) return;
    if (_landRevision != scene.land->revision) {
        _uploadLand(*scene.land);
    }
    if (!_sunIndexCount) {
        _uploadSun();
    }
    vkWaitForFences(_device, 1, &_frameFence, VK_TRUE, UINT64_MAX);
    std::uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
        _imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) { _recreateSwapchain(); return; }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swapchain image");
    vkResetFences(_device, 1, &_frameFence);
    vkResetCommandBuffer(_commandBuffer, 0);
    const VkCommandBufferBeginInfo begin{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    require(vkBeginCommandBuffer(_commandBuffer, &begin), "Failed to begin command buffer");

    const glm::mat4 lightViewProjection = _lightViewProjection(scene.sun);
    const ShadowState shadowState{lightViewProjection};
    void* shadowUniformData = nullptr;
    require(
        vkMapMemory(
            _device,
            _shadowUniformBuffer.memory,
            0,
            sizeof(shadowState),
            0,
            &shadowUniformData
        ),
        "Failed to map shadow uniform memory"
    );
    std::memcpy(shadowUniformData, &shadowState, sizeof(shadowState));
    vkUnmapMemory(_device, _shadowUniformBuffer.memory);

    const VkImageMemoryBarrier shadowWriteBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = _shadowMapInitialized
            ? static_cast<VkAccessFlags>(VK_ACCESS_SHADER_READ_BIT)
            : VkAccessFlags{},
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = _shadowMapInitialized
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _shadowImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vkCmdPipelineBarrier(
        _commandBuffer,
        _shadowMapInitialized
            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &shadowWriteBarrier
    );
    const VkClearValue shadowDepthClear{.depthStencil = {1.0F, 0}};
    const VkRenderingAttachmentInfo shadowDepth{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = _shadowView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = shadowDepthClear,
    };
    const VkRenderingInfo shadowRendering{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, {shadowMapSize, shadowMapSize}},
        .layerCount = 1,
        .pDepthAttachment = &shadowDepth,
    };
    vkCmdBeginRendering(_commandBuffer, &shadowRendering);
    vkCmdBindPipeline(
        _commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _shadowPipeline
    );
    const VkViewport shadowViewport{
        0.0F,
        0.0F,
        static_cast<float>(shadowMapSize),
        static_cast<float>(shadowMapSize),
        0.0F,
        1.0F,
    };
    const VkRect2D shadowScissor{{0, 0}, {shadowMapSize, shadowMapSize}};
    vkCmdSetViewport(_commandBuffer, 0, 1, &shadowViewport);
    vkCmdSetScissor(_commandBuffer, 0, 1, &shadowScissor);
    constexpr VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(
        _commandBuffer, 0, 1, &_vertexBuffer.handle, &offset
    );
    vkCmdBindIndexBuffer(
        _commandBuffer, _indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32
    );
    const DrawState shadowDrawState{
        lightViewProjection,
        glm::vec4{0.0F},
        glm::vec4{0.0F},
        glm::vec4{0.0F},
    };
    vkCmdPushConstants(
        _commandBuffer,
        _shadowPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(shadowDrawState),
        &shadowDrawState
    );
    vkCmdDrawIndexed(_commandBuffer, _indexCount, 1, 0, 0, 0);
    vkCmdEndRendering(_commandBuffer);
    const VkImageMemoryBarrier shadowReadBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _shadowImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vkCmdPipelineBarrier(
        _commandBuffer,
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &shadowReadBarrier
    );
    _shadowMapInitialized = true;

    const std::array barriers{
        VkImageMemoryBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = _swapchainImages[imageIndex], .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}},
        VkImageMemoryBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = _depthImage, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}}};
    vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0, 0, nullptr, 0, nullptr, static_cast<std::uint32_t>(barriers.size()), barriers.data());
    const float daylight = scene.sun.intensity;
    constexpr std::array nightSky{0.008F, 0.015F, 0.055F};
    constexpr std::array daySky{0.28F, 0.62F, 0.88F};
    const VkClearValue colorClear{.color = {{
        nightSky[0] + (daySky[0] - nightSky[0]) * daylight,
        nightSky[1] + (daySky[1] - nightSky[1]) * daylight,
        nightSky[2] + (daySky[2] - nightSky[2]) * daylight,
        1.0F,
    }}};
    const VkClearValue depthClear{.depthStencil = {1.0F, 0}};
    const VkRenderingAttachmentInfo color{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = _swapchainViews[imageIndex], .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = colorClear};
    const VkRenderingAttachmentInfo depth{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = _depthView, .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = depthClear};
    const VkRenderingInfo rendering{.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, _swapchainExtent}, .layerCount = 1, .colorAttachmentCount = 1,
        .pColorAttachments = &color, .pDepthAttachment = &depth};
    vkCmdBeginRendering(_commandBuffer, &rendering);
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vkCmdBindDescriptorSets(
        _commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipelineLayout,
        0,
        1,
        &_shadowDescriptorSet,
        0,
        nullptr
    );
    const VkViewport viewport{0, 0, static_cast<float>(_swapchainExtent.width),
        static_cast<float>(_swapchainExtent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, _swapchainExtent};
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &_vertexBuffer.handle, &offset);
    vkCmdBindIndexBuffer(_commandBuffer, _indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
    const glm::mat4 viewProjection = scene.camera.projection * scene.camera.view;
    const glm::vec4 lightingState{scene.sun.direction, scene.sun.intensity};
    const DrawState landState{
        viewProjection,
        glm::vec4{0.0F},
        lightingState,
        glm::vec4{scene.temperatureOverlay ? 1.0F : 0.0F, 0.0F, 0.0F, 0.0F},
    };
    vkCmdPushConstants(_commandBuffer, _pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(landState), &landState);
    vkCmdDrawIndexed(_commandBuffer, _indexCount, 1, 0, 0, 0);
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &_sunVertexBuffer.handle, &offset);
    vkCmdBindIndexBuffer(_commandBuffer, _sunIndexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
    const DrawState sunDrawState{
        viewProjection,
        glm::vec4{scene.sun.position, 1.0F},
        glm::vec4{scene.sun.direction, scene.sun.intensity},
        glm::vec4{0.0F},
    };
    vkCmdPushConstants(_commandBuffer, _pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(sunDrawState), &sunDrawState);
    vkCmdDrawIndexed(_commandBuffer, _sunIndexCount, 1, 0, 0, 0);
    vkCmdEndRendering(_commandBuffer);
    const VkImageMemoryBarrier presentBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _swapchainImages[imageIndex], .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
    vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    require(vkEndCommandBuffer(_commandBuffer), "Failed to record command buffer");
    constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1, .pWaitSemaphores = &_imageAvailable, .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1, .pCommandBuffers = &_commandBuffer,
        .signalSemaphoreCount = 1, .pSignalSemaphores = &_renderFinished};
    require(vkQueueSubmit(_graphicsQueue, 1, &submit, _frameFence), "Failed to submit frame");
    const VkPresentInfoKHR present{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1, .pWaitSemaphores = &_renderFinished,
        .swapchainCount = 1, .pSwapchains = &_swapchain, .pImageIndices = &imageIndex};
    result = vkQueuePresentKHR(_graphicsQueue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) _recreateSwapchain();
    else if (result != VK_SUCCESS) throw std::runtime_error("Failed to present frame");
}

void VulkanSystem::_uploadLand(const Land& land) {
    if (_vertexBuffer.handle != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
        vkDestroyBuffer(_device, _vertexBuffer.handle, nullptr);
        vkFreeMemory(_device, _vertexBuffer.memory, nullptr);
        vkDestroyBuffer(_device, _indexBuffer.handle, nullptr);
        vkFreeMemory(_device, _indexBuffer.memory, nullptr);
        _vertexBuffer = {};
        _indexBuffer = {};
    }
    VkDeviceSize vertexSize = sizeof(Vertex) * land.vertices.size();
    VkDeviceSize indexSize = sizeof(std::uint32_t) * land.indices.size();
    _vertexBuffer = _createBuffer(vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    _indexBuffer = _createBuffer(indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    void* data;
    vkMapMemory(_device, _vertexBuffer.memory, 0, vertexSize, 0, &data);
    std::memcpy(data, land.vertices.data(), vertexSize);
    vkUnmapMemory(_device, _vertexBuffer.memory);
    vkMapMemory(_device, _indexBuffer.memory, 0, indexSize, 0, &data);
    std::memcpy(data, land.indices.data(), indexSize);
    vkUnmapMemory(_device, _indexBuffer.memory);
    _indexCount = static_cast<std::uint32_t>(land.indices.size());
    _landRevision = land.revision;
    glm::vec3 minimum{std::numeric_limits<float>::max()};
    glm::vec3 maximum{std::numeric_limits<float>::lowest()};
    for (const Vertex& vertex : land.vertices) {
        minimum.x = std::min(minimum.x, vertex.position.x);
        minimum.y = std::min(minimum.y, vertex.position.y);
        minimum.z = std::min(minimum.z, vertex.position.z);
        maximum.x = std::max(maximum.x, vertex.position.x);
        maximum.y = std::max(maximum.y, vertex.position.y);
        maximum.z = std::max(maximum.z, vertex.position.z);
    }
    _landCenter = (minimum + maximum) * 0.5F;
    _landRadius = std::max(glm::length(maximum - minimum) * 0.55F, 1.0F);
}

glm::mat4 VulkanSystem::_lightViewProjection(const Sun& sun) const {
    const glm::vec3 lightDirection = glm::normalize(sun.direction);
    const glm::vec3 up = std::abs(glm::dot(lightDirection, glm::vec3{0, 1, 0})) >
            0.95F
        ? glm::vec3{0.0F, 0.0F, 1.0F}
        : glm::vec3{0.0F, 1.0F, 0.0F};
    const glm::vec3 lightPosition =
        _landCenter + lightDirection * (_landRadius * 2.5F);
    const glm::mat4 view = glm::lookAt(lightPosition, _landCenter, up);
    glm::mat4 projection = glm::orthoRH_ZO(
        -_landRadius,
        _landRadius,
        -_landRadius,
        _landRadius,
        0.1F,
        _landRadius * 5.0F
    );
    projection[1][1] *= -1.0F;
    return projection * view;
}

void VulkanSystem::_uploadSun() {
    constexpr glm::vec3 sunlight{1.0F, 0.82F, 0.18F};
    constexpr float radius = 0.7F;
    const std::array vertices{
        Vertex{{0.0F, radius, 0.0F}, sunlight},
        Vertex{{radius, 0.0F, 0.0F}, sunlight},
        Vertex{{0.0F, 0.0F, radius}, sunlight},
        Vertex{{-radius, 0.0F, 0.0F}, sunlight},
        Vertex{{0.0F, 0.0F, -radius}, sunlight},
        Vertex{{0.0F, -radius, 0.0F}, sunlight},
    };
    constexpr std::array<std::uint32_t, 24> indices{
        0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1,
        5, 2, 1, 5, 3, 2, 5, 4, 3, 5, 1, 4,
    };
    const VkDeviceSize vertexSize = sizeof(vertices);
    const VkDeviceSize indexSize = sizeof(indices);
    _sunVertexBuffer = _createBuffer(vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    _sunIndexBuffer = _createBuffer(indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    void* data = nullptr;
    vkMapMemory(_device, _sunVertexBuffer.memory, 0, vertexSize, 0, &data);
    std::memcpy(data, vertices.data(), vertexSize);
    vkUnmapMemory(_device, _sunVertexBuffer.memory);
    vkMapMemory(_device, _sunIndexBuffer.memory, 0, indexSize, 0, &data);
    std::memcpy(data, indices.data(), indexSize);
    vkUnmapMemory(_device, _sunIndexBuffer.memory);
    _sunIndexCount = static_cast<std::uint32_t>(indices.size());
}

VulkanSystem::Buffer VulkanSystem::_createBuffer(VkDeviceSize size, VkBufferUsageFlags usage) const {
    Buffer buffer;
    const VkBufferCreateInfo info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size, .usage = usage, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
    require(vkCreateBuffer(_device, &info, nullptr, &buffer.handle), "Failed to create land buffer");
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(_device, buffer.handle, &requirements);
    const VkMemoryAllocateInfo allocation{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size, .memoryTypeIndex = _findMemoryType(requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
    require(vkAllocateMemory(_device, &allocation, nullptr, &buffer.memory), "Failed to allocate land memory");
    require(vkBindBufferMemory(_device, buffer.handle, buffer.memory, 0), "Failed to bind land memory");
    return buffer;
}

std::uint32_t VulkanSystem::_findMemoryType(std::uint32_t allowed, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties available{};
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &available);
    for (std::uint32_t index = 0; index < available.memoryTypeCount; ++index)
        if ((allowed & (1U << index)) &&
            (available.memoryTypes[index].propertyFlags & properties) == properties) return index;
    throw std::runtime_error("No suitable Vulkan memory type");
}

VkShaderModule VulkanSystem::_loadShader(const std::string& path) const {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Could not open compiled shader: " + path);
    auto size = stream.tellg();
    if (size <= 0 || size % 4) throw std::runtime_error("Invalid compiled shader: " + path);
    std::vector<std::uint32_t> code(static_cast<std::size_t>(size) / 4);
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(code.data()), size);
    const VkShaderModuleCreateInfo info{.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<std::size_t>(size), .pCode = code.data()};
    VkShaderModule shader;
    require(vkCreateShaderModule(_device, &info, nullptr, &shader), "Failed to create shader module");
    return shader;
}

void VulkanSystem::_destroySwapchain() {
    if (_pipeline) {
        vkDestroyPipeline(_device, _pipeline, nullptr);
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        _pipeline = VK_NULL_HANDLE;
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_shadowPipeline) {
        vkDestroyPipeline(_device, _shadowPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _shadowPipelineLayout, nullptr);
        _shadowPipeline = VK_NULL_HANDLE;
        _shadowPipelineLayout = VK_NULL_HANDLE;
    }
    if (_depthView) {
        vkDestroyImageView(_device, _depthView, nullptr);
        vkDestroyImage(_device, _depthImage, nullptr);
        vkFreeMemory(_device, _depthMemory, nullptr);
        _depthView = VK_NULL_HANDLE;
    }
    for (auto view : _swapchainViews) vkDestroyImageView(_device, view, nullptr);
    _swapchainViews.clear();
    _swapchainImages.clear();
    if (_swapchain) {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;
    }
}

void VulkanSystem::_destroyShadowResources() {
    if (_shadowDescriptorPool) {
        vkDestroyDescriptorPool(_device, _shadowDescriptorPool, nullptr);
    }
    if (_shadowDescriptorLayout) {
        vkDestroyDescriptorSetLayout(
            _device, _shadowDescriptorLayout, nullptr
        );
    }
    if (_shadowUniformBuffer.handle) {
        vkDestroyBuffer(_device, _shadowUniformBuffer.handle, nullptr);
        vkFreeMemory(_device, _shadowUniformBuffer.memory, nullptr);
    }
    if (_shadowSampler) {
        vkDestroySampler(_device, _shadowSampler, nullptr);
    }
    if (_shadowView) {
        vkDestroyImageView(_device, _shadowView, nullptr);
    }
    if (_shadowImage) {
        vkDestroyImage(_device, _shadowImage, nullptr);
        vkFreeMemory(_device, _shadowMemory, nullptr);
    }
}

void VulkanSystem::_recreateSwapchain() {
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    if (!width || !height) return;
    vkDeviceWaitIdle(_device);
    _destroySwapchain();
    _createSwapchain();
}
