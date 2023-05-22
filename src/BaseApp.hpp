#ifndef BASE_APP_HPP
#define BASE_APP_HPP

#include <string_view>

class GLFWwindow;

struct BaseApp
{
private:
protected:
    // GLFWwindow* glfw_window { nullptr };
    // bool headless = false;
public:
    BaseApp( const std::string_view config_file_path );
    ~BaseApp();
};

#endif // BASE_APP_HPP