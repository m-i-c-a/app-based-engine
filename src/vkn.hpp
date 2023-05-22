#ifndef VKN_HPP
#define VKN_HPP

#include <vulkan/vulkan.h>
#include <assert.h>
#include <string>
#include <stdio.h>
#include <vector>

#define VK_CHECK(val)                  \
    do                                 \
    {                                  \
        if (val != VK_SUCCESS)         \
        {                              \
            assert(val == VK_SUCCESS); \
        }                              \
    } while (false)

#define EXIT(fmt, ...)                       \
    do                                       \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
        assert(false);                       \
    } while (0)

#define VKN_DEBUG

class GLFWwindow;

namespace vkn
{
    typedef uint32_t ResourceID;

struct InitInfo
{
    void* window;
    uint32_t win_width;
    uint32_t win_height;
};

struct CoreInfo
{
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceMemoryProperties physical_device_mem_props;
};

struct VkResource
{
private:
    static ResourceID id_counter;
protected:
    ResourceID id = -1;
    std::string name = "no_name";
public:
    VkResource( )
        : id { id_counter++ }
        , name { "no_name" }
    {
    }

    VkResource( std::string_view debug_name )
        : id { id_counter++ }
        , name { debug_name }
    {
    }

    ResourceID get_id() const { return id; }
    std::string_view get_name() const { return name; }
};

struct Buffer : public VkResource
{
    VkBuffer handle = VK_NULL_HANDLE;

    Buffer( )
        : VkResource( )
    {}

    Buffer( std::string_view debug_name )
        : VkResource( debug_name )
    {}
};

struct Memory : public VkResource
{
    VkDeviceMemory handle = VK_NULL_HANDLE;

    Memory( )
        : VkResource( )
    {}

    Memory( std::string_view debug_name )
        : VkResource( debug_name )
    {}
};


struct CommandBuffer
{
    VkCommandBuffer handle { VK_NULL_HANDLE };
};

bool get_headless();
GLFWwindow* get_glfw_window();

uint32_t get_frames_in_flight();
std::vector<VkImage> get_swapchain_images();
std::vector<VkImageView> get_swapchain_image_views();

void present( const VkQueue queue, const uint32_t swapchain_image_index, const uint32_t wait_semaphore_count = 0, const VkSemaphore* const wait_semaphores = nullptr, void* p_next = nullptr );

void request_features( );

void init( const std::string_view json_path );
void destroy();

void device_wait_idle();

uint32_t acquire_next_image( const uint64_t timeout, const VkSemaphore semaphore, const VkFence fence );

VkQueue get_queue( const uint32_t index );
uint32_t get_queue_family_index();

VkBuffer create_buffer( const VkBufferUsageFlags buffer_usage, const VkDeviceSize size );
VkBuffer create_buffer( const VkBufferCreateInfo& create_info );
void destroy_buffer( const VkBuffer buffer );

VkDeviceMemory alloc_buffer_memory( const VkBuffer buffer, const VkMemoryPropertyFlags mem_props );
void bind_buffer_memory( const VkBuffer buffer, const VkDeviceMemory memory, const VkDeviceSize offset );
void map_memory( const VkDeviceMemory memory, const uint64_t offset, const VkDeviceSize size, void** data );
void unmap_memory( const VkDeviceMemory memory );
void free_memory( const VkDeviceMemory memory );

VkShaderModule create_shader_module( const std::string_view file_path );
void destroy_shader_module( const VkShaderModule module );

VkPipelineLayout create_pipeline_layout( const VkPipelineLayoutCreateInfo& create_info );
void destroy_pipeline_layout( const VkPipelineLayout layout );

VkPipeline create_compute_pipeline( const VkComputePipelineCreateInfo& create_info );
void destroy_pipeline( const VkPipeline pipeline );

VkDescriptorSetLayout create_desc_set_layout( const uint32_t binding_count, const VkDescriptorSetLayoutBinding* const bindings, const VkDescriptorSetLayoutCreateFlags flags = 0x0, const void* const p_next = nullptr );
void destroy_desc_set_layout( const VkDescriptorSetLayout layout );

VkDescriptorPool create_desc_pool( const uint32_t max_sets, const uint32_t pool_size_count, const VkDescriptorPoolSize* const pool_sizes, const VkDescriptorPoolCreateFlags flags = 0x0, const void* const p_next = nullptr );
void destroy_desc_pool( const VkDescriptorPool pool );

VkDescriptorSet alloc_desc_set( const VkDescriptorPool pool, const VkDescriptorSetLayout* const layout, const void* const p_next = nullptr );
std::vector<VkDescriptorSet> alloc_desc_sets( const VkDescriptorPool pool, const uint32_t count, const VkDescriptorSetLayout* const layout, const void* const p_next = nullptr );

void write_desc_sets( const uint32_t write_count, const VkWriteDescriptorSet* write_desc_sets );

VkCommandPool create_command_pool( const VkCommandPoolCreateFlags flags );
void reset_command_pool( const VkCommandPool pool );
void destroy_command_pool( const VkCommandPool pool );

VkCommandBuffer allocate_command_buffer( const VkCommandPool cmd_pool, const VkCommandBufferLevel level );
std::vector<VkCommandBuffer> allocate_command_buffers( const VkCommandPool cmd_pool, const VkCommandBufferLevel level, const uint32_t count );

VkFence create_fence( const VkFenceCreateFlags flags = 0x0, const void* p_next = nullptr );
void wait_for_fence( const VkFence fence, const uint64_t timeout );
void wait_for_fences( );
void reset_fence( const VkFence fence );
void reset_fences( );
void destroy_fence( const VkFence fence );

}; // vkn

#endif // VKN_HPP