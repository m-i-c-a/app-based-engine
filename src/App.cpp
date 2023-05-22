#include "App.hpp"
#include "vkn.hpp"
#include "StagingBuffer.hpp"
#include "Buffer.hpp"
#include "defines.hpp"

#include <string.h>

static constexpr uint32_t num_elements_to_sum = ( 10 << 20 );

App::App( const std::string_view config_file_path )
    : WindowedApp( config_file_path )
    , queue { vkn::get_queue( 0 ) }
    , cmd_pool { vkn::create_command_pool( 0x0 ) }
    , cmd_buff { vkn::allocate_command_buffer( cmd_pool , VK_COMMAND_BUFFER_LEVEL_PRIMARY ) }
{
    // We require an initial transition from layout UNDEFINED -> PRESENT
    transition_swapchain_images();
    init_resources();

    WindowedApp::set_present_queue( queue );
    WindowedApp::run();
}

App::~App()
{
    vkn::destroy_command_pool( cmd_pool );
    vkn::destroy_pipeline( pipeline );
    vkn::destroy_pipeline_layout( pipeline_layout );
    vkn::destroy_desc_set_layout( desc_set_layout );
    vkn::destroy_desc_pool( desc_pool );
}

void App::transition_swapchain_images()
{
    std::vector<VkImage> swapchain_images = vkn::get_swapchain_images();
    
    std::vector<VkImageMemoryBarrier> swapchain_image_barriers;
    swapchain_image_barriers.reserve( swapchain_images.size() );

    for ( const VkImage image : swapchain_images )
    {
        const VkImageMemoryBarrier transition_to_present_barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = vkn::get_queue_family_index(),
            .dstQueueFamilyIndex = vkn::get_queue_family_index(),
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        swapchain_image_barriers.push_back( transition_to_present_barrier );
    }

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    VK_CHECK( vkBeginCommandBuffer( cmd_buff, &cmd_buff_begin_info ) );

    vkCmdPipelineBarrier( 
        cmd_buff, 
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr,
        0, nullptr,
        static_cast<uint32_t>( swapchain_image_barriers.size() ), swapchain_image_barriers.data() );

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

    vkn::reset_command_pool( cmd_pool );
}

void App::init_resources()
{

    staging_buffer = std::make_unique<StagingBuffer>( ( 1 << 20 ) * 50 );
    device_local_input_buffer = std::make_unique<const Buffer>( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, num_elements_to_sum * sizeof( uint32_t ) );
    device_local_output_buffer = std::make_unique<const Buffer>( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof( uint32_t ) );
    host_output_buffer = std::make_unique<const Buffer>( VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof( uint32_t ) );

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

    desc_set_layout = vkn::create_desc_set_layout( static_cast<uint32_t>( desc_set_bindings.size() ), desc_set_bindings.data() );

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .setLayoutCount = 1,
        .pSetLayouts = &desc_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    pipeline_layout = vkn::create_pipeline_layout( pipeline_layout_create_info );

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

    pipeline = vkn::create_compute_pipeline( pipeline_create_info );

    vkn::destroy_shader_module( pipeline_create_info.stage.module );

    const std::array<VkDescriptorPoolSize, 1> desc_pool_sizes {{
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2,
        }
    }};

    desc_pool = vkn::create_desc_pool( 1, static_cast<uint32_t>( desc_pool_sizes.size() ), desc_pool_sizes.data() );
    desc_set = vkn::alloc_desc_set( desc_pool, &desc_set_layout );

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

    // Upload data to input buffer

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

void App::execute_frame() 
{
    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    vkn::reset_command_pool( cmd_pool );
    VK_CHECK( vkBeginCommandBuffer( cmd_buff, &cmd_buff_begin_info ) );

    // upload zeros
    {
        static constexpr uint32_t zero = 0;
        staging_buffer->queue_upload( device_local_output_buffer->buffer, 0, sizeof( uint32_t ), &zero );
        staging_buffer->record_flush( cmd_buff );

        void* cpu_data;
        vkn::map_memory( host_output_buffer->memory, 0, sizeof( uint32_t ), &cpu_data );
        memcpy( cpu_data, &zero, sizeof( uint32_t ) );
        vkn::unmap_memory( host_output_buffer->memory );
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

        vkCmdDispatch( cmd_buff, num_elements_to_sum / 32, 1, 1 );  
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

        vkCmdCopyBuffer( cmd_buff, device_local_output_buffer->buffer, host_output_buffer->buffer, 1, &buff_copy );
    }

    VK_CHECK( vkEndCommandBuffer( cmd_buff ) );

    // submit
    {
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

    void* cpu_data;
    uint32_t sum = 0;
    vkn::map_memory( host_output_buffer->memory, 0, sizeof( uint32_t ), &cpu_data );
    memcpy( &sum, cpu_data, sizeof( uint32_t ) );
    vkn::unmap_memory( host_output_buffer->memory );

    LOG("Sum: %u\n", sum);
}