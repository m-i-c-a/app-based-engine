#include "StagingBuffer.hpp"
#include "Buffer.hpp"
#include "vkn.hpp"
#include "defines.hpp"

#include <string.h>

StagingBuffer::StagingBuffer( const VkDeviceSize buffer_size )
    : size { buffer_size }
    , buffer { std::make_unique<const Buffer>( VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size ) }
{
    vkn::map_memory( buffer->memory, 0, size, (void**)(&mapped_ptr) );
}

void StagingBuffer::queue_upload( const VkBuffer dst_buffer, const VkDeviceSize dst_buffer_offset, const VkDeviceSize upload_size, const void* const data )
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

void StagingBuffer::record_flush( const VkCommandBuffer cmd_buff )
{
    for ( const auto& [dst_buffer, uploads] : queued_buffer_upload_infos )
    {
        vkCmdCopyBuffer( cmd_buff, buffer->buffer, dst_buffer, static_cast<uint32_t>( uploads.size() ), uploads.data() );
    }

    queued_buffer_upload_infos.clear();
    offset = 0;
}