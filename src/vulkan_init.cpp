#include "vulkan_init.hpp"
#include "defines.hpp"

#include "json_parser/json.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <optional>
#include <unordered_set>
#include <vector>

struct ConfigInfoInstance
{
    std::string application_name; 
    uint32_t application_version[3];
    std::string engine_name;
    uint32_t engine_version[3];
    uint32_t api_version[2];
    std::vector<std::string> layers;
    std::vector<std::string> extensions;
};

void from_json(const nlohmann::json& j, ConfigInfoInstance& c)
{
    j.at("application_name").get_to(c.application_name);
    j.at("application_version").get_to(c.application_version);
    j.at("engine_name").get_to(c.engine_name);
    j.at("engine_version").get_to(c.engine_version);
    j.at("api_version").get_to(c.api_version);
    j.at("layers").get_to(c.layers);
    j.at("extensions").get_to(c.extensions);
}

struct ConfigInfoDevice
{
    std::vector<std::vector<std::string>> queues;
    std::vector<std::string> layers;
    std::vector<std::string> extensions;
};

void from_json(const nlohmann::json& j, ConfigInfoDevice& c)
{
    j.at("queues").get_to(c.queues);
    j.at("layers").get_to(c.layers);
    j.at("extensions").get_to(c.extensions);
}

struct ConfigInfoSwapchain
{
    uint32_t image_width = 0;
    uint32_t image_height = 0;
    uint32_t min_image_count = 0;
    std::string present_mode;
    uint32_t frames_in_flight = 0;
};

void from_json(const nlohmann::json& j, ConfigInfoSwapchain& c)
{
    j.at("image_width").get_to(c.image_width);
    j.at("image_height").get_to(c.image_height);
    j.at("min_image_count").get_to(c.min_image_count);
    j.at("present_mode").get_to(c.present_mode);
    j.at("frames_in_flight").get_to(c.frames_in_flight);
}

static GLFWwindow* init_glfw( const nlohmann::json& json_data )
{ 
    const ConfigInfoSwapchain config_info = json_data.at( "swapchain" ).get<ConfigInfoSwapchain>();

    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    GLFWwindow *glfw_window = glfwCreateWindow( static_cast<int>( config_info.image_width ), static_cast<int>( config_info.image_height ), "", nullptr, nullptr );
    assert( glfw_window && "Failed to create window" );
    return glfw_window;
}

static VkInstance create_instance( const nlohmann::json& json_data )
{
    const ConfigInfoInstance config_info = json_data.at("instance").get<ConfigInfoInstance>();

    const uint32_t api_version = [](const uint32_t versions[2]){
        switch (versions[0])
        {
            case 1:
            {
                switch (versions[1])
                {
                    case 0:
                        return VK_API_VERSION_1_0;
                    case 1:
                        return VK_API_VERSION_1_1;
                    case 2:
                        return VK_API_VERSION_1_2;
                    case 3:
                        return VK_API_VERSION_1_3;
                    default:
                        EXIT("Invalid VK_VERSION specified in config file.\n");
                }
                break;
            }
            default:
            {
                EXIT("Invalid VK_VERSION specified in config file.\n");
            }
        }
        return 0u;
    }(config_info.api_version);

    std::vector<const char*> layers (config_info.layers.size(), "");
    for (uint32_t i = 0; i < layers.size(); ++i)
        layers[i] = config_info.layers.at(i).c_str();

    std::vector<const char*> extensions (config_info.extensions.size(), "");
    for (uint32_t i = 0; i < extensions.size(); ++i)
        extensions[i] = config_info.extensions.at(i).c_str();

    const VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config_info.application_name.c_str(),
        .applicationVersion = VK_MAKE_VERSION(config_info.application_version[0], config_info.application_version[1], config_info.application_version[2]),
        .pEngineName = config_info.engine_name.c_str(),
        .engineVersion = VK_MAKE_VERSION(config_info.engine_version[0], config_info.engine_version[1], config_info.engine_version[2]),
        .apiVersion = api_version
    };

    const VkInstanceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));
    return instance;
}

static VkSurfaceKHR create_surface( const VkInstance instance, GLFWwindow* const window )
{
    VkSurfaceKHR surface { VK_NULL_HANDLE };
    VK_CHECK( glfwCreateWindowSurface(instance, window, nullptr, &surface ) );
    return surface;
}

static VkPhysicalDevice select_physical_device( const VkInstance instance )
{
    uint32_t num_physical_devices = 0;
    vkEnumeratePhysicalDevices( instance, &num_physical_devices, nullptr );
    std::vector<VkPhysicalDevice> physical_devices( num_physical_devices );
    vkEnumeratePhysicalDevices( instance, &num_physical_devices, physical_devices.data() );

    auto queue_flags_to_str = [&]( const VkQueueFlags flags )
    {
        std::stringstream ss;

        if ( flags & VK_QUEUE_GRAPHICS_BIT )
            ss << "GRAPHICS ";
        if ( flags & VK_QUEUE_COMPUTE_BIT )
            ss << "COMPUTE ";
        if ( flags & VK_QUEUE_TRANSFER_BIT )
            ss << "TRANSFER ";
        if ( flags & VK_QUEUE_SPARSE_BINDING_BIT )
            ss << "SPARSE_BINDING ";    

        return ss.str();
    };

    for ( uint32_t i = 0; i < num_physical_devices; i++ )
    {
        VkPhysicalDeviceProperties physical_device_props;
        vkGetPhysicalDeviceProperties( physical_devices[i], &physical_device_props );

        std::stringstream ss;

        ss << "Device ID " << i << '\n';
        ss << "\tName: " << physical_device_props.deviceName << '\n';
        ss << "\tType: " << physical_device_props.deviceType << '\n';

        uint32_t num_queue_family_props = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( physical_devices[i], &num_queue_family_props, nullptr );
        std::vector<VkQueueFamilyProperties> queue_family_props( num_queue_family_props );
        vkGetPhysicalDeviceQueueFamilyProperties( physical_devices[i], &num_queue_family_props, queue_family_props.data() );

        ss << "\tNum Queue Families: " << num_queue_family_props << '\n';

        for ( uint32_t j = 0; j < num_queue_family_props; j++ )
        {
            ss << "\t\tQueue Family " << j << '\n';
            ss << "\t\t\tQueue Count: " << queue_family_props[i].queueCount << '\n';
            ss << "\t\t\tQueue Flags: " << queue_flags_to_str( queue_family_props[i].queueFlags ) << '\n';
        }

        LOG( "%s\n", ss.str().c_str() );
    }

    const uint32_t idx = 0;
    LOG("Selecting Physical Device: %u\n", idx);

    return physical_devices[idx];
}

static uint32_t select_queue_family_index( const nlohmann::json& json_data, const VkPhysicalDevice physical_device, const std::optional<VkSurfaceKHR> surface )
{   
    uint32_t num_queue_family_props = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &num_queue_family_props, nullptr );
    std::vector<VkQueueFamilyProperties> queue_family_props( num_queue_family_props );
    vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &num_queue_family_props, queue_family_props.data() );

    bool present_requested = false;
    const VkQueueFlags requested_queue_flags = [&]()
    {
        VkQueueFlags flags = 0x0;
        for ( const std::vector<std::string>& queue_flag_list : json_data.at("device").at("queues") ) 
        {
            for ( const std::string& queue_flag_str : queue_flag_list )
            {
                if ( queue_flag_str == "GRAPHICS" )
                    flags |= VK_QUEUE_GRAPHICS_BIT;
                if ( queue_flag_str == "COMPUTE" )
                    flags |= VK_QUEUE_COMPUTE_BIT;
                if ( queue_flag_str == "TRANSFER" )
                    flags |= VK_QUEUE_TRANSFER_BIT;
                if ( queue_flag_str == "SPARSE" )
                    flags |= VK_QUEUE_SPARSE_BINDING_BIT;
                if ( queue_flag_str == "PRESENT" )
                    present_requested = true;
            }
        }

        return flags;
    }();

    if ( present_requested )
        assert( surface.has_value() );

    if ( surface.has_value() )
        assert( present_requested );

    uint32_t i = 0;
    for ( ; i < queue_family_props.size(); i++ )
    {
        const VkQueueFamilyProperties& props = queue_family_props[i];

        if ( ( props.queueFlags & requested_queue_flags ) == requested_queue_flags )
        {
            // We have found a queue family that supports all of our requests (excluding present)

            if ( present_requested )
            {
                VkBool32 queue_family_supports_present = false;
                VK_CHECK( vkGetPhysicalDeviceSurfaceSupportKHR( physical_device, i, surface.value(), &queue_family_supports_present ) );

                if ( queue_family_supports_present )
                    break;
            }
            else
                break;
        }
    }

    LOG( "Selecing Queue Family Index: %u\n", i );
    return i;
}

static VkDevice create_device( const nlohmann::json& json_data, const VkPhysicalDevice physical_device, const uint32_t queue_family_idx, uint32_t& requested_queue_count )
{
    const ConfigInfoDevice config_info = json_data.at("device").get<ConfigInfoDevice>();

    std::vector<const char*> layers (config_info.layers.size(), "");
    for (uint32_t i = 0; i < layers.size(); ++i)
        layers[i] = config_info.layers.at(i).c_str();

    std::vector<const char*> extensions(config_info.extensions.size(), "");
    for (uint32_t i = 0; i < extensions.size(); ++i)
        extensions[i] = config_info.extensions.at(i).c_str();

    const std::vector<float> queue_priorities( config_info.queues.size(), 1.0f );

    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_idx,
        .queueCount = static_cast<uint32_t>( config_info.queues.size() ),
        .pQueuePriorities = queue_priorities.data()
    };

    requested_queue_count = queue_create_info.queueCount;

    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = (layers.size() == 0) ? nullptr : layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = nullptr
    };

    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device));
    return device;
}

static std::vector<VkQueue> get_queues( const VkDevice device, const uint32_t queue_family_index, const uint32_t queue_count )
{
   std::vector<VkQueue> queues( queue_count, VK_NULL_HANDLE );
   for ( uint32_t i = 0; i < queue_count; i++ )
   {
       vkGetDeviceQueue( device, queue_family_index, i, &queues[i] );
   }
   return queues;
}


static uint32_t getSwapchainMinImageCount(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities, const uint32_t requestedImageCount)
{
    assert(requestedImageCount > 0 && "Invalid requested image count for swapchain!");

    uint32_t minImageCount = UINT32_MAX;

    // If the maxImageCount is 0, then there is not a limit on the number of images the swapchain
    // can support (ignoring memory constraints). See the Vulkan Spec for more information.

    if (vk_surfaceCapabilities.maxImageCount == 0)
    {
        if (requestedImageCount >= vk_surfaceCapabilities.minImageCount)
        {
            minImageCount = requestedImageCount;
        }
        else
        {
            EXIT("Failed to create Swapchain. The requested number of images %u does not meet the minimum requirement of %u.\n", requestedImageCount, vk_surfaceCapabilities.minImageCount);
        }
    }
    else if (requestedImageCount >= vk_surfaceCapabilities.minImageCount && requestedImageCount <= vk_surfaceCapabilities.maxImageCount)
    {
        minImageCount = requestedImageCount;
    }
    else
    {
        EXIT("The number of requested Swapchain images %u is not supported. Min: %u Max: %u.\n", requestedImageCount, vk_surfaceCapabilities.minImageCount, vk_surfaceCapabilities.maxImageCount);
    }

    return minImageCount;
}

static void getSwapchainImageFormatAndColorSpace(const VkPhysicalDevice vk_physicalDevice, const VkSurfaceKHR vk_surface, const VkFormat requestedFormat, VkFormat& chosenFormat, VkColorSpaceKHR& chosenColorSpace)
{
    uint32_t numSupportedSurfaceFormats = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physicalDevice, vk_surface, &numSupportedSurfaceFormats, nullptr));
    VkSurfaceFormatKHR* supportedSurfaceFormats = new VkSurfaceFormatKHR[numSupportedSurfaceFormats];
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physicalDevice, vk_surface, &numSupportedSurfaceFormats, supportedSurfaceFormats));

    bool requestedFormatFound = false;
    for (uint32_t i = 0; i < numSupportedSurfaceFormats; ++i)
    {
        if (supportedSurfaceFormats[i].format == requestedFormat)
        {
            chosenFormat = supportedSurfaceFormats[i].format;
            chosenColorSpace = supportedSurfaceFormats[i].colorSpace;
            requestedFormatFound = true;
            break;
        }
    }

    if (!requestedFormatFound)
    {
        chosenFormat = supportedSurfaceFormats[0].format;
        chosenColorSpace = supportedSurfaceFormats[0].colorSpace;
    }

    delete [] supportedSurfaceFormats;
}

static VkExtent2D getSwapchainExtent(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities, const VkExtent2D vk_requestedImageExtent)
{
    VkExtent2D vk_extent;
    
    // The Vulkan Spec states that if the current width/height is 0xFFFFFFFF, then the surface size
    // will be deteremined by the extent specified in the VkSwapchainCreateInfoKHR.

    if (vk_surfaceCapabilities.currentExtent.width != (uint32_t)-1)
    {
        vk_extent = vk_requestedImageExtent;
    }
    else
    {
        vk_extent = vk_surfaceCapabilities.currentExtent;
    }

    return vk_extent;
}

static VkSurfaceTransformFlagBitsKHR getSwapchainPreTransform(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities)
{
    VkSurfaceTransformFlagBitsKHR vk_preTransform = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;

    if (vk_surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        vk_preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        vk_preTransform = vk_surfaceCapabilities.currentTransform;
        LOG("WARNING - Swapchain pretransform is not IDENTITIY_BIT_KHR!\n");
    }

    return vk_preTransform;
}

static VkCompositeAlphaFlagBitsKHR getSwapchainCompositeAlpha(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities)
{
    VkCompositeAlphaFlagBitsKHR vk_compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;

    // Determine the composite alpha format the application needs.
    // Find a supported composite alpha format (not all devices support alpha opaque),
    // but we prefer it.
    // Simply select the first composite alpha format available
    // Used for blending with other windows in the system

    const VkCompositeAlphaFlagBitsKHR vk_compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (size_t i = 0; i < 4; ++i)
    {
        if (vk_surfaceCapabilities.supportedCompositeAlpha & vk_compositeAlphaFlags[i]) 
        {
            vk_compositeAlpha = vk_compositeAlphaFlags[i];
            break;
        };
    }

    return vk_compositeAlpha;
}

static VkPresentModeKHR getSwapchainPresentMode(const VkPhysicalDevice vk_physicalDevice, const VkSurfaceKHR vk_surface, const VkPresentModeKHR vk_requestedPresentMode)
{
    VkPresentModeKHR vk_presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    uint32_t numSupportedPresentModes = 0;
    VkPresentModeKHR* supportedPresentModes = nullptr;

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physicalDevice, vk_surface, &numSupportedPresentModes, nullptr));
    supportedPresentModes = new VkPresentModeKHR[numSupportedPresentModes];
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physicalDevice, vk_surface, &numSupportedPresentModes, supportedPresentModes));

    // Determine the present mode the application needs.
    // Try to use mailbox, it is the lowest latency non-tearing present mode
    // All devices support FIFO (this mode waits for the vertical blank or v-sync)

    vk_presentMode = VK_PRESENT_MODE_FIFO_KHR;

    for (uint32_t i = 0; i < numSupportedPresentModes; ++i)
    {
        if (supportedPresentModes[i] == vk_requestedPresentMode)
        {
            vk_presentMode = vk_requestedPresentMode;
            break;
        }
    }

    delete [] supportedPresentModes;
    return vk_presentMode;
}


static VkSwapchainCreateInfoKHR populate_swapchain_create_info(const nlohmann::json& json_data, const VkPhysicalDevice vk_physicalDevice, const VkSurfaceKHR vk_surface, const VkDevice vk_device )
{
    const ConfigInfoSwapchain config_info = json_data.at( "swapchain" ).get<ConfigInfoSwapchain>();

    const VkFormat requested_image_format = VK_FORMAT_R8G8B8_SRGB;
    const VkExtent2D requested_extent { .width = config_info.image_width, .height = config_info.image_height };
    const VkPresentModeKHR requested_present_mode = []( const std::string_view str )
    {
        if (str == "IMMEDIATE")
        {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        if (str == "MAILBOX")
        {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        if (str == "FIFO_RELAXED")
        {
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }

        if (str != "FIFO")
        {
            EXIT("Invalid present mode specified in config file: %s\n", str.data());
        }

        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }( config_info.present_mode );

    VkSurfaceCapabilitiesKHR vk_surfaceCapabilities;
    VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk_physicalDevice, vk_surface, &vk_surfaceCapabilities ) );

    VkFormat vk_imageFormat;
    VkColorSpaceKHR vk_imageColorSpace;
    getSwapchainImageFormatAndColorSpace( vk_physicalDevice, vk_surface, requested_image_format, vk_imageFormat, vk_imageColorSpace );

    VkSwapchainCreateInfoKHR vk_swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_surface,
        .minImageCount = getSwapchainMinImageCount(vk_surfaceCapabilities, config_info.min_image_count),
        .imageFormat = vk_imageFormat,
        .imageColorSpace = vk_imageColorSpace,
        .imageExtent = getSwapchainExtent(vk_surfaceCapabilities, requested_extent),
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = getSwapchainPreTransform(vk_surfaceCapabilities),
        .compositeAlpha = getSwapchainCompositeAlpha(vk_surfaceCapabilities),
        .presentMode = getSwapchainPresentMode(vk_physicalDevice, vk_surface, requested_present_mode),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    return vk_swapchainCreateInfo;
}


static VkSwapchainKHR create_swapchain(const VkDevice vk_device, const VkSwapchainCreateInfoKHR& vk_swapchainCreateInfo)
{
    VkSwapchainKHR vk_swapchain;
    VK_CHECK(vkCreateSwapchainKHR(vk_device, &vk_swapchainCreateInfo, nullptr, &vk_swapchain));
    return vk_swapchain;
}

static std::vector<VkImage> get_swapchain_images(const VkDevice vk_device, const VkSwapchainKHR vk_swapchain)
{
    uint32_t numSwapchainImages = 0;
    std::vector<VkImage> vk_swapchainImages;

    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &numSwapchainImages, nullptr));
    vk_swapchainImages.resize(numSwapchainImages);
    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &numSwapchainImages, vk_swapchainImages.data()));

    return vk_swapchainImages;
}

static std::vector<VkImageView> create_swapchain_image_views(const VkDevice vk_device, const std::vector<VkImage>& vk_swapchainImages, const VkFormat vk_swapchainImageFormat)
{
    std::vector<VkImageView> vk_swapchainImageViews(vk_swapchainImages.size());

    VkImageViewCreateInfo vk_imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_swapchainImageFormat,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 },
    };

    for (size_t i = 0; i < vk_swapchainImages.size(); ++i)
    {
        vk_imageViewCreateInfo.image = vk_swapchainImages[i];
        VK_CHECK(vkCreateImageView(vk_device, &vk_imageViewCreateInfo, nullptr, &vk_swapchainImageViews[i]));
    }

    LOG("Swapchain Image Count: %lu\n", vk_swapchainImages.size());
    return vk_swapchainImageViews;
}



VulkanCoreInfo vulkan_init( const std::string_view json_path )
{
    std::ifstream file( json_path.data() );
    ASSERT( file.is_open(), "Failed to open init config file: %s\n", json_path.data() );

    const nlohmann::json json_data = nlohmann::json::parse(file);

    file.close();

    // We do not require a window/surface/swapchain. This is to support headless or compute-only applications.
    std::optional<SwapchainInfo> swapchain_info = ( json_data.find( "swapchain" ) == json_data.end() ) ? std::nullopt : std::make_optional<SwapchainInfo>();

    const VkInstance instance = create_instance( json_data );
    
    if ( swapchain_info.has_value() )
    {
        swapchain_info->glfw_window = init_glfw( json_data );
        swapchain_info->surface = create_surface( instance, swapchain_info->glfw_window );
    }

    const VkPhysicalDevice physical_device = select_physical_device( instance );

    // We currently only support single queue family applications. You can however, create and use multiple 
    // queues within the same queue family.
    const uint32_t queue_family_index = select_queue_family_index( json_data, physical_device, swapchain_info.has_value() ? std::make_optional<VkSurfaceKHR>( swapchain_info->surface ) : std::nullopt ); 

    uint32_t requested_queue_count = 0;
    const VkDevice device = create_device( json_data, physical_device, queue_family_index, requested_queue_count );
    std::vector<VkQueue> queues = get_queues( device, queue_family_index, requested_queue_count );

    if ( swapchain_info.has_value() )
    {
        const VkSwapchainCreateInfoKHR swapchain_create_info = populate_swapchain_create_info( json_data, physical_device, swapchain_info->surface, device );

        swapchain_info->swapchain_image_format = swapchain_create_info.imageFormat;
        swapchain_info->swapchain_image_extent = swapchain_create_info.imageExtent;
        swapchain_info->swapchain = create_swapchain( device, swapchain_create_info );
        swapchain_info->swapchain_images = get_swapchain_images( device, swapchain_info->swapchain );
        swapchain_info->swapchain_image_views = create_swapchain_image_views( device, swapchain_info->swapchain_images, swapchain_info->swapchain_image_format );

        swapchain_info->frames_in_flight = json_data.at( "swapchain" ).at( "frames_in_flight" ).get<uint32_t>();
    }

    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    const VulkanCoreInfo core_info {
        .instance = instance,
        .physical_device = physical_device,
        .queue_family_index = queue_family_index,
        .device = device,
        .queues = queues,
        .swapchain_info = swapchain_info,
        .physical_device_memory_properties = physical_device_memory_properties
    };

    return core_info;
}