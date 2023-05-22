#include "vkn.hpp"
#include "defines.hpp"

#include <GLFW/glfw3.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <string.h>
#include <array>
#include <optional>


struct Buffer
{
    const VkDeviceSize size { 0 };
    const VkBuffer buffer { VK_NULL_HANDLE };
    const VkDeviceMemory memory { VK_NULL_HANDLE };
    const bool own_memory { false };

    Buffer( const VkBufferUsageFlags usage_flags, const VkMemoryPropertyFlags memory_flags, const VkDeviceSize _size )
        : size { _size }
        , buffer { vkn::create_buffer( usage_flags, size ) }
        , memory { vkn::alloc_buffer_memory( buffer, memory_flags ) }
        , own_memory { true }
    {
        vkn::bind_buffer_memory( buffer, memory, 0 );
    }

    Buffer( const VkBufferUsageFlags usage_flags, const VkDeviceMemory _memory, const VkDeviceSize offset, const VkDeviceSize _size )
        : size { _size }
        , buffer { vkn::create_buffer( usage_flags, size ) }
        , memory { _memory }
        , own_memory { false }
    {
        vkn::bind_buffer_memory( buffer, memory, offset );
    }

    ~Buffer()
    {
        vkn::destroy_buffer( buffer );

        if ( own_memory )
        {
            vkn::free_memory( memory );
        }
    }
};

struct StagingBuffer
{
private:
    const VkDeviceSize size = 0lu;  
    const std::unique_ptr<const Buffer> buffer { nullptr };

    uint8_t* mapped_ptr = nullptr;
    VkDeviceSize offset = 0lu;

    std::unordered_map<VkBuffer, std::vector<VkBufferCopy>> queued_buffer_upload_infos;
public:
    StagingBuffer( const VkDeviceSize buffer_size )
        : size { buffer_size }
        , buffer { std::make_unique<const Buffer>( VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size ) }
    {
        vkn::map_memory( buffer->memory, 0, size, (void**)(&mapped_ptr) );
    }

    void queue_upload( const VkBuffer dst_buffer, const VkDeviceSize dst_buffer_offset, const VkDeviceSize upload_size, const void* const data )
    {
        if ( offset + upload_size >= size )
        {
            EXIT("Attempting to upload more data than staging buffer can store!\n");
        }

        memcpy(mapped_ptr + offset, data, upload_size);

        const VkBufferCopy buff_copy {
            .srcOffset = offset,
            .dstOffset = dst_buffer_offset,
            .size = upload_size
        };
        queued_buffer_upload_infos[dst_buffer].push_back( std::move( buff_copy ) );

        offset += upload_size;
    }

    void record_flush( const VkCommandBuffer cmd_buff )
    {
        for ( const auto& [dst_buffer, uploads] : queued_buffer_upload_infos )
        {
            vkCmdCopyBuffer( cmd_buff, buffer->buffer, dst_buffer, static_cast<uint32_t>( uploads.size() ), uploads.data() );
        }

        queued_buffer_upload_infos.clear();
        offset = 0;
    }
};


static std::unique_ptr<StagingBuffer> staging_buffer = nullptr;

static std::unique_ptr<const Buffer> device_local_input_buffer = nullptr;
static std::unique_ptr<const Buffer> device_local_output_buffer = nullptr;
static std::unique_ptr<const Buffer> cpu_output_buffer = nullptr;

static constexpr uint32_t num_elements_to_sum = 32;

void upload_data( const VkCommandBuffer cmd_buff, const VkQueue queue )
{
    std::vector<uint32_t> buffer_data( num_elements_to_sum, 1 );

    staging_buffer->queue_upload( device_local_input_buffer->buffer, 0, num_elements_to_sum * sizeof( uint32_t ), buffer_data.data() );

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    VK_CHECK( vkBeginCommandBuffer( cmd_buff, &cmd_buff_begin_info ) );

    staging_buffer->record_flush( cmd_buff );

    VK_CHECK( vkEndCommandBuffer( cmd_buff ) );

    const VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buff,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    vkQueueSubmit( queue, 1, &submit_info, VK_NULL_HANDLE );

    vkn::device_wait_idle();
}

void run()
{ 
    VkQueue compute_queue = vkn::get_queue( 0 );

    const VkCommandPool cmd_pool = vkn::create_command_pool( 0x0 );
    const VkCommandBuffer cmd_buff = vkn::allocate_command_buffer( cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY );

    staging_buffer = std::make_unique<StagingBuffer>( ( 1 << 20 ) * 50 );
    device_local_input_buffer = std::make_unique<const Buffer>( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, num_elements_to_sum * sizeof( uint32_t ) );
    device_local_output_buffer = std::make_unique<const Buffer>( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof( uint32_t ) );
    cpu_output_buffer = std::make_unique<const Buffer>( VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof( uint32_t ) );

    upload_data( cmd_buff, compute_queue );

    const std::array<VkDescriptorSetLayoutBinding, 2> desc_set_bindings {{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        }
    }};

    const VkDescriptorSetLayout desc_set_layout = vkn::create_desc_set_layout( static_cast<uint32_t>( desc_set_bindings.size() ), desc_set_bindings.data() );

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .setLayoutCount = 1,
        .pSetLayouts = &desc_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    const VkPipelineLayout pipeline_layout = vkn::create_pipeline_layout( pipeline_layout_create_info );

    VkComputePipelineCreateInfo pipeline_create_info {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = vkn::create_shader_module( "array_sum.comp" ),
            .pName = "main",
            .pSpecializationInfo = nullptr },
        .layout = pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    const VkPipeline pipeline = vkn::create_compute_pipeline( pipeline_create_info );


    const std::array<VkDescriptorPoolSize, 1> desc_pool_sizes {{
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2,
        }
    }};

    const VkDescriptorPool desc_pool = vkn::create_desc_pool( 1, static_cast<uint32_t>( desc_pool_sizes.size() ), desc_pool_sizes.data() );
    const VkDescriptorSet desc_set = vkn::alloc_desc_set( desc_pool, &desc_set_layout );

    const std::array<VkDescriptorBufferInfo, 2> desc_buffer_infos {{
        {
            .buffer = device_local_input_buffer->buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        },
        {
            .buffer = device_local_output_buffer->buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        },
    }};

    const VkWriteDescriptorSet write_desc_set {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = desc_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = desc_buffer_infos.data(),
        .pTexelBufferView = nullptr,
    };

    vkn::write_desc_sets( 1, &write_desc_set );

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    while ( true )
    {
        vkn::reset_command_pool( cmd_pool );
        VK_CHECK( vkBeginCommandBuffer( cmd_buff, &cmd_buff_begin_info ) );

        // upload zeros
        {
            static constexpr uint32_t zero = 0;
            staging_buffer->queue_upload( device_local_output_buffer->buffer, 0, sizeof( uint32_t ), &zero );
            staging_buffer->record_flush( cmd_buff );

            void* cpu_data;
            vkn::map_memory( cpu_output_buffer->memory, 0, sizeof( uint32_t ), &cpu_data );
            memcpy( cpu_data, &zero, sizeof( uint32_t ) );
            vkn::unmap_memory( cpu_output_buffer->memory );
        }

        // barrier
        {
            const VkBufferMemoryBarrier buff_mem_barrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = vkn::get_queue_family_index(),
                .dstQueueFamilyIndex = vkn::get_queue_family_index(),
                .buffer = device_local_output_buffer->buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };

            vkCmdPipelineBarrier( cmd_buff, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                0x0,
                0, nullptr,
                1, &buff_mem_barrier,
                0, nullptr );
        }

        // dispatch
        {
            vkCmdBindPipeline( cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline );

            vkCmdBindDescriptorSets( cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &desc_set, 0, nullptr );

            vkCmdDispatch( cmd_buff, 1, 1, 1 );  
        }

        // barrier
        {
            const VkBufferMemoryBarrier buff_mem_barrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .srcQueueFamilyIndex = vkn::get_queue_family_index(),
                .dstQueueFamilyIndex = vkn::get_queue_family_index(),
                .buffer = device_local_output_buffer->buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };

            vkCmdPipelineBarrier( cmd_buff, 
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0x0,
                0, nullptr,
                1, &buff_mem_barrier,
                0, nullptr );
        }

        // GPU -> CPU copy
        {
            const VkBufferCopy buff_copy {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = sizeof( uint32_t )
            };

            vkCmdCopyBuffer( cmd_buff, device_local_output_buffer->buffer, cpu_output_buffer->buffer, 1, &buff_copy );
        }

        VK_CHECK( vkEndCommandBuffer( cmd_buff ) );

        const VkSubmitInfo submit_info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buff,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };

        vkQueueSubmit( compute_queue, 1, &submit_info, VK_NULL_HANDLE );

        vkn::device_wait_idle();

        void* cpu_data;
        uint32_t sum = 0;
        vkn::map_memory( cpu_output_buffer->memory, 0, sizeof( uint32_t ), &cpu_data );
        memcpy( &sum, cpu_data, sizeof( uint32_t ) );
        vkn::unmap_memory( cpu_output_buffer->memory );

        LOG("Sum: %u\n", sum);
    }

    device_local_input_buffer.reset();
    device_local_output_buffer.reset();
    cpu_output_buffer.reset();

    vkn::destroy_desc_pool( desc_pool );
    vkn::destroy_desc_set_layout( desc_set_layout );

    vkn::destroy_command_pool( cmd_pool );
    vkn::destroy_shader_module( pipeline_create_info.stage.module );
    vkn::destroy_pipeline_layout( pipeline_layout );
    vkn::destroy_pipeline( pipeline );
}

int main()
{
    App app( "/home/mica/Desktop/Vulkan/compute/data/json/vulkan_info.json" );

    // vkn::init( "/home/mica/Desktop/Vulkan/compute/data/json/vulkan_info.json" );

    // run();

    // vkn::destroy();

    return 0;
}