#include "vkn.hpp"
#include "vulkan_init.hpp"
#include "defines.hpp"

#include <GLFW/glfw3.h>

namespace vkn
{

namespace
{

static VulkanCoreInfo core;

static uint32_t get_memory_type_idx(const uint32_t memory_type_indices, const VkMemoryPropertyFlags memory_property_flags)
{
   	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < core.physical_device_memory_properties.memoryTypeCount; i++)
	{
		if (memory_type_indices & (1 << i) && (core.physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags)
		{
			return i;
		}
	}

    EXIT("Could not find suitable memory type!");
    return 0; 
}

}

uint32_t VkResource::id_counter = 0;


bool get_headless()
{
    return !core.swapchain_info.has_value();
}

GLFWwindow* get_glfw_window()
{
    assert( core.swapchain_info.has_value() );
    return core.swapchain_info->glfw_window;
}


uint32_t get_frames_in_flight()
{
    assert( core.swapchain_info.has_value() );
    return core.swapchain_info->frames_in_flight;
}


std::vector<VkImage> get_swapchain_images()
{
    assert( core.swapchain_info.has_value() );
    return core.swapchain_info->swapchain_images;
}

std::vector<VkImageView> get_swapchain_image_views()
{
    assert( core.swapchain_info.has_value() );
    return core.swapchain_info->swapchain_image_views;
}


void present( const VkQueue queue, const uint32_t swapchain_image_index, const uint32_t wait_semaphore_count, const VkSemaphore* const wait_semaphores, void* p_next )
{
    assert( core.swapchain_info.has_value() );

    const VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = p_next,
        .waitSemaphoreCount = wait_semaphore_count,
        .pWaitSemaphores = wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &core.swapchain_info->swapchain,
        .pImageIndices = &swapchain_image_index,
        .pResults = nullptr,
    };

    VK_CHECK( vkQueuePresentKHR( queue, &present_info ) );
}


void init( const std::string_view json_path )
{
    core = vulkan_init( json_path );
}

void destroy()
{
    if ( core.swapchain_info.has_value() )
    {
        glfwDestroyWindow( core.swapchain_info->glfw_window );
        glfwTerminate();

        for ( uint32_t i = 0; i < core.swapchain_info->swapchain_image_views.size(); i++ )
            vkDestroyImageView( core.device, core.swapchain_info->swapchain_image_views[i], nullptr );

        vkDestroySwapchainKHR( core.device, core.swapchain_info->swapchain, nullptr );
        vkDestroySurfaceKHR( core.instance, core.swapchain_info->surface, nullptr );
    }

    vkDestroyDevice( core.device, nullptr );
    vkDestroyInstance( core.instance, nullptr );
}


void device_wait_idle()
{
    VK_CHECK( vkDeviceWaitIdle( core.device ) );
}

uint32_t acquire_next_image( const uint64_t timeout, const VkSemaphore semaphore, const VkFence fence )
{
    assert( core.swapchain_info.has_value() );
    uint32_t image_index = 0;
    VK_CHECK( vkAcquireNextImageKHR( core.device, core.swapchain_info->swapchain, timeout, semaphore, fence, &image_index ) );
    return image_index;
}

VkQueue get_queue( const uint32_t index )
{
    return core.queues.at( index );
}

uint32_t get_queue_family_index()
{
    return core.queue_family_index;
}

VkBuffer create_buffer( const VkBufferCreateInfo& create_info )
{
    VkBuffer buffer { VK_NULL_HANDLE };
    VK_CHECK( vkCreateBuffer( core.device, &create_info, nullptr, &buffer ) );
    return buffer;
}

VkBuffer create_buffer( const VkBufferUsageFlags buffer_usage, const VkDeviceSize size )
{
    const VkBufferCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .size = static_cast<VkDeviceSize>( size ),
        .usage = buffer_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkBuffer buffer { VK_NULL_HANDLE };
    VK_CHECK( vkCreateBuffer( core.device, &create_info, nullptr, &buffer ) );
    return buffer;
}

void destroy_buffer( const VkBuffer buffer )
{
    vkDestroyBuffer( core.device, buffer, nullptr );
}


VkDeviceMemory alloc_buffer_memory( const VkBuffer buffer, const VkMemoryPropertyFlags mem_props )
{
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements( core.device, buffer, &mem_reqs );

    const VkMemoryAllocateInfo mem_alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = get_memory_type_idx( mem_reqs.memoryTypeBits, mem_props )
    };

    VkDeviceMemory memory { VK_NULL_HANDLE };
    VK_CHECK( vkAllocateMemory( core.device, &mem_alloc_info, nullptr, &memory ) );
    return memory;
}

void bind_buffer_memory( const VkBuffer buffer, const VkDeviceMemory memory, const VkDeviceSize offset )
{
    VK_CHECK( vkBindBufferMemory( core.device, buffer, memory, offset ) );
}

void map_memory( const VkDeviceMemory memory, const uint64_t offset, const uint64_t size, void** data )
{
    VK_CHECK( vkMapMemory( core.device, memory, offset, size, 0x0, data ) );
}

void unmap_memory( const VkDeviceMemory memory )
{
    vkUnmapMemory( core.device, memory );
}

void free_memory( const VkDeviceMemory memory )
{
    vkFreeMemory( core.device, memory, nullptr );
}


VkShaderModule create_shader_module( const std::string_view file_path )
{
    static const std::string prefix = "/home/mica/Desktop/Vulkan/compute/data/spirv/";
    static const std::string postfix = ".spv";

    std::string complete_filepath = prefix + std::string( file_path ) + postfix;

    FILE* f = fopen( complete_filepath.c_str(), "r" );
    ASSERT( f != 0, "Failed to open file %s!\n", complete_filepath.c_str() );

    fseek( f, 0, SEEK_END );
    const size_t nbytes_file_size = (size_t)ftell( f );
    rewind( f );

    uint32_t* buffer = (uint32_t*)malloc( nbytes_file_size );
    fread( buffer, nbytes_file_size, 1, f );
    fclose( f );

    VkShaderModuleCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = nbytes_file_size,
        .pCode = buffer,
    };

    VkShaderModule shader_module = VK_NULL_HANDLE;
    VK_CHECK( vkCreateShaderModule( core.device, &create_info, NULL, &shader_module ) );

    free( buffer );
    return shader_module;
}

void destroy_shader_module( const VkShaderModule module )
{
    vkDestroyShaderModule( core.device, module, nullptr );
}

VkPipelineLayout create_pipeline_layout( const VkPipelineLayoutCreateInfo& create_info )
{
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VK_CHECK( vkCreatePipelineLayout( core.device, &create_info, nullptr, &pipeline_layout ) );
    return pipeline_layout;
}

void destroy_pipeline_layout( const VkPipelineLayout layout )
{
    vkDestroyPipelineLayout( core.device, layout, nullptr );
}

VkPipeline create_compute_pipeline( const VkComputePipelineCreateInfo& create_info )
{
    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_CHECK( vkCreateComputePipelines( core.device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline ) );
    return pipeline;
}

void destroy_pipeline( const VkPipeline pipeline )
{
    vkDestroyPipeline( core.device, pipeline, nullptr );
}

VkDescriptorSetLayout create_desc_set_layout( const uint32_t binding_count, const VkDescriptorSetLayoutBinding* const bindings, const VkDescriptorSetLayoutCreateFlags flags, const void* const p_next )
{
    const VkDescriptorSetLayoutCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = p_next,
        .flags = flags,
        .bindingCount = binding_count,
        .pBindings = bindings
    };

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK( vkCreateDescriptorSetLayout( core.device, &create_info, nullptr, &layout ) );
    return layout;
}

void destroy_desc_set_layout( const VkDescriptorSetLayout layout )
{
    vkDestroyDescriptorSetLayout( core.device, layout, nullptr ); 
}

VkDescriptorPool create_desc_pool( const uint32_t max_sets, const uint32_t pool_size_count, const VkDescriptorPoolSize* const pool_sizes, const VkDescriptorPoolCreateFlags flags, const void* const p_next )
{
  const VkDescriptorPoolCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = p_next,
        .flags = flags,
        .maxSets = max_sets,
        .poolSizeCount = pool_size_count,
        .pPoolSizes = pool_sizes
    };

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VK_CHECK( vkCreateDescriptorPool( core.device, &create_info, nullptr, &pool ) );
    return pool;
}

void destroy_desc_pool( const VkDescriptorPool pool )
{
    vkDestroyDescriptorPool( core.device, pool, nullptr );
}

VkDescriptorSet alloc_desc_set( const VkDescriptorPool pool, const VkDescriptorSetLayout* const layout, const void* const p_next )
{
    const VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = p_next,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = layout,
    };

    VkDescriptorSet set = VK_NULL_HANDLE;
    VK_CHECK( vkAllocateDescriptorSets( core.device, &alloc_info, &set ) );
    return set;
}

std::vector<VkDescriptorSet> alloc_desc_sets( const VkDescriptorPool pool, const uint32_t count, const VkDescriptorSetLayout* const layout, const void* const p_next )
{
    const VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = p_next,
        .descriptorPool = pool,
        .descriptorSetCount = count,
        .pSetLayouts = layout,
    };

    std::vector<VkDescriptorSet> sets( count, VK_NULL_HANDLE );
    VK_CHECK( vkAllocateDescriptorSets( core.device, &alloc_info, sets.data() ) );
    return sets;
}

void write_desc_sets( const uint32_t write_count, const VkWriteDescriptorSet* write_desc_sets )
{
    vkUpdateDescriptorSets( core.device, write_count, write_desc_sets, 0, nullptr );
}

VkCommandPool create_command_pool( const VkCommandPoolCreateFlags flags )
{
    const VkCommandPoolCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = core.queue_family_index,
    };

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VK_CHECK( vkCreateCommandPool( core.device, &create_info, nullptr, &cmd_pool ) );
    return cmd_pool;
}

void reset_command_pool( const VkCommandPool pool )
{
    VK_CHECK( vkResetCommandPool( core.device, pool, 0x0 ) );
}

void destroy_command_pool( const VkCommandPool pool )
{
    vkDestroyCommandPool( core.device, pool, nullptr );
}

VkCommandBuffer allocate_command_buffer( const VkCommandPool cmd_pool, const VkCommandBufferLevel level )
{
    VkCommandBufferAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cmd_pool,
        .level = level,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd_buff = VK_NULL_HANDLE;
    VK_CHECK( vkAllocateCommandBuffers( core.device, &alloc_info, &cmd_buff ) );
    return cmd_buff;
}


VkFence create_fence( const VkFenceCreateFlags flags, const void* p_next )
{
    const VkFenceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = p_next,
        .flags = flags,
    }; 

    VkFence fence { VK_NULL_HANDLE };
    VK_CHECK( vkCreateFence( core.device, &create_info, nullptr, &fence ) );
    return fence;
}

void wait_for_fence( const VkFence fence, const uint64_t timeout )
{
    VK_CHECK( vkWaitForFences( core.device, 1, &fence, VK_TRUE, timeout ) );
}

void reset_fence( const VkFence fence )
{
    VK_CHECK( vkResetFences( core.device, 1, &fence ) );
}

void destroy_fence( const VkFence fence )
{
    vkDestroyFence( core.device, fence, nullptr );
}

}; // vkn