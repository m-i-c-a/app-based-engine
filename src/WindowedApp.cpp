#include "WindowedApp.hpp"
#include "vkn.hpp"

#include <GLFW/glfw3.h>
#include <assert.h>

WindowedApp::WindowedApp( const std::string_view config_file_path )
    : BaseApp( config_file_path )
    , resource_count { vkn::get_frames_in_flight() }
    , image_acquire_fences { vkn::get_frames_in_flight(), vkn::create_fence() }
    , glfw_window { vkn::get_glfw_window() }
    , active_resource_index { 0 }
{
    assert( vkn::get_headless() == false );
}

void WindowedApp::run()
{
    assert( present_queue != VK_NULL_HANDLE );

    while ( !glfwWindowShouldClose( glfw_window ) )
    {
        glfwPollEvents();

        const uint32_t active_swapchain_image_index = vkn::acquire_next_image( UINT64_MAX, VK_NULL_HANDLE, image_acquire_fences[active_resource_index] );
        vkn::wait_for_fence( image_acquire_fences[active_resource_index], UINT64_MAX );
        vkn::reset_fence( image_acquire_fences[active_resource_index] );        

        execute_frame();

        vkn::present( present_queue, active_swapchain_image_index );

        vkn::device_wait_idle();
    }
}

WindowedApp::~WindowedApp()
{
    for ( uint32_t i = 0; i < resource_count; i++ )
    {
        vkn::destroy_fence( image_acquire_fences[i] );
    }
}