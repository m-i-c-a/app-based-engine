#ifndef WINDOWED_APP_HPP
#define WINDOWED_APP_HPP

#include "BaseApp.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <string_view>

class GLFWwindow;

struct WindowedApp : public BaseApp
{
private:
    const uint32_t resource_count { 0 };
    const std::vector<VkFence> image_acquire_fences;
    GLFWwindow* const glfw_window { nullptr };
    uint32_t active_resource_index { 0 };
    VkQueue present_queue { VK_NULL_HANDLE };
protected:
    void set_present_queue( const VkQueue queue ) { present_queue = queue; }
    void run();

    virtual void execute_frame() = 0;
public:
    WindowedApp( const std::string_view config_file_path );
    ~WindowedApp();
};


#endif // Windowed_APP_HPP