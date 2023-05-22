#ifndef STAGING_BUFFER_HPP
#define STAGING_BUFFER_HPP

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

class Buffer;

struct StagingBuffer
{
private:
    const VkDeviceSize size = 0lu;  
    const std::unique_ptr<const Buffer> buffer { nullptr };

    uint8_t* mapped_ptr = nullptr;
    VkDeviceSize offset = 0lu;

    std::unordered_map<VkBuffer, std::vector<VkBufferCopy>> queued_buffer_upload_infos;
public:
    StagingBuffer( const VkDeviceSize buffer_size );
    void queue_upload( const VkBuffer dst_buffer, const VkDeviceSize dst_buffer_offset, const VkDeviceSize upload_size, const void* const data );
    void record_flush( const VkCommandBuffer cmd_buff );
};

#endif // STAGING_BUFFER_HPP