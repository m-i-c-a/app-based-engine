#include "BaseApp.hpp"
#include "vkn.hpp"

BaseApp::BaseApp( const std::string_view config_file_path )
{
    vkn::init( config_file_path );

    // headless = vkn::get_headless();

    // if ( !headless )
    // {
    //     glfw_window = vkn::get_glfw_window();
    // }
}

BaseApp::~BaseApp()
{
    vkn::destroy();
}