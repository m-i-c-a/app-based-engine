#ifndef APP_HPP
#define APP_HPP

#include "WindowedApp.hpp"

#include <vulkan/vulkan.h>
#include <memory>

class Buffer;
class StagingBuffer;

class App : public WindowedApp
{
private:
    const VkQueue queue;
    const VkCommandPool cmd_pool;
    const VkCommandBuffer cmd_buff;

    VkDescriptorSetLayout desc_set_layout { VK_NULL_HANDLE };
    VkDescriptorSet desc_set { VK_NULL_HANDLE };
    VkDescriptorPool desc_pool { VK_NULL_HANDLE };
    VkPipelineLayout pipeline_layout { VK_NULL_HANDLE };
    VkPipeline pipeline { VK_NULL_HANDLE };

    std::unique_ptr<StagingBuffer> staging_buffer { nullptr };

    std::unique_ptr<const Buffer> device_local_input_buffer { nullptr };
    std::unique_ptr<const Buffer> device_local_output_buffer { nullptr };
    std::unique_ptr<const Buffer> host_output_buffer { nullptr };

    void transition_swapchain_images();
    void init_resources();

    virtual void execute_frame() override final;
public:
    App( const std::string_view config_file_path );
    ~App();
};

#endif // APP_HPP