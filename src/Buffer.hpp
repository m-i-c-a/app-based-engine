#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.h>

struct Buffer
{
    const VkDeviceSize size { 0 };
    const VkBuffer buffer { VK_NULL_HANDLE };
    const VkDeviceMemory memory { VK_NULL_HANDLE };
    const bool own_memory { false };

    Buffer( const VkBufferUsageFlags usage_flags, const VkMemoryPropertyFlags memory_flags, const VkDeviceSize _size );
    Buffer( const VkBufferUsageFlags usage_flags, const VkDeviceMemory _memory, const VkDeviceSize offset, const VkDeviceSize _size );
    ~Buffer();
};

#endif // BUFFER_HPP