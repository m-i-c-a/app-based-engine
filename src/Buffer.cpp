#include "Buffer.hpp"
#include "vkn.hpp"

Buffer::Buffer( const VkBufferUsageFlags usage_flags, const VkMemoryPropertyFlags memory_flags, const VkDeviceSize _size )
    : size { _size }
    , buffer { vkn::create_buffer( usage_flags, size ) }
    , memory { vkn::alloc_buffer_memory( buffer, memory_flags ) }
    , own_memory { true }
{
    vkn::bind_buffer_memory( buffer, memory, 0 );
}

Buffer::Buffer( const VkBufferUsageFlags usage_flags, const VkDeviceMemory _memory, const VkDeviceSize offset, const VkDeviceSize _size )
    : size { _size }
    , buffer { vkn::create_buffer( usage_flags, size ) }
    , memory { _memory }
    , own_memory { false }
{
    vkn::bind_buffer_memory( buffer, memory, offset );
}

Buffer::~Buffer()
{
    vkn::destroy_buffer( buffer );

    if ( own_memory )
    {
        vkn::free_memory( memory );
    }
}