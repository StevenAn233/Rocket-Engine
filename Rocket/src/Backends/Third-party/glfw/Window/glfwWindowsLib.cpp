module;

#include <glfw/glfw3.h>

module WindowsLib;

import Log;
import RenderBackend;

namespace {
    static void error_callback(int error, const char* description)
        { CORE_ERROR(u8"GLFW: ERROR({}), {}!", error, description); }
}

namespace rke
{
    NativeWindow WindowsLib::get_current_context()
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return NativeWindow(glfwGetCurrentContext());
            break;
        default:
            CORE_ASSERT(false, u8"glfwWindowLib: Other APIs not supported!");
        }
        return NativeWindow();
    }

    void WindowsLib::make_context_current(NativeWindow context)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            glfwMakeContextCurrent(context.as<GLFWwindow>());
            break;
        default:
            CORE_ASSERT(false, u8"glfwWindowLib: Other APIs not supported!");
        }
    }

    bool WindowsLib::is_context_current(NativeWindow context)
        { return get_current_context() == context; }

    WindowsLib::WindowsLib()
    {
        CORE_ASSERT(glfwInit(), u8"glfw: Failed to initialize GLFW!");
        glfwSetErrorCallback(error_callback);
    }

    WindowsLib::~WindowsLib()
    {
        glfwTerminate();
        CORE_INFO(u8"glfwWindowsLib: GLFW terminated.");
    }

    void WindowsLib::refresh()
    {
        if(!main_window_) return;
        glfwPollEvents();

        if(main_window_->should_close()) {
            map_.clear(); main_window_ = nullptr;
        } else {
            std::erase_if(map_, [this](auto& pair)
            {
                if(pair.second->should_close()) return true;
                return false;
            });
        }
        
        for(auto& [_, window] : map_)
        {
            switch(render_backend::get_graphics_api())
            {
            case GraphicsAPI::OpenGL:
                glfwSwapBuffers(window->get_context().as<GLFWwindow>());
                break;
            default:
                CORE_ASSERT(false, u8"glfwWindow: Other API not supported!");
            }
        }
    }
}
