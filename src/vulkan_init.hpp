#ifndef VULKAN_INIT_HPP
#define VULKAN_INIT_HPP

#include <vulkan/vulkan.h>

#include <string_view>
#include <vector>
#include <optional>

class GLFWwindow;

struct SwapchainInfo
{
    GLFWwindow* glfw_window { nullptr };
    VkSurfaceKHR surface { VK_NULL_HANDLE };
    VkSwapchainKHR swapchain { VK_NULL_HANDLE };
    VkFormat swapchain_image_format { VK_FORMAT_UNDEFINED };
    VkExtent2D swapchain_image_extent { 0, 0 };
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    uint32_t frames_in_flight { 0 };
};

struct VulkanCoreInfo
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t queue_family_index = UINT32_MAX;
    VkDevice device = VK_NULL_HANDLE;
    std::vector<VkQueue> queues;

    std::optional<SwapchainInfo> swapchain_info { std::nullopt };

    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
};

VulkanCoreInfo vulkan_init( const std::string_view json_path );

#endif // VULKAN_INIT_HPP