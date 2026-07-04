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
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return NativeWindow(glfwGetCurrentContext());
            break;
        default:
            CORE_ASSERT(false, u8"glfwWindow: Other API not supported!");
        }
        return NativeWindow();
    }

    void WindowsLib::make_context_current(NativeWindow context)
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            glfwMakeContextCurrent(context.as<GLFWwindow>());
            break;
        default:
            CORE_ASSERT(false, u8"glfwWindow: Other API not supported!");
        }
    }

    WindowsLib::WindowsLib(std::function<void(Window&)> callback)
        : load_callback_(std::move(callback))
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
        std::erase_if(map_, [this](auto& pair)
        {
            Window& window{ *(pair.second.get()) };
            if(window.should_close()) return true;

            switch(RenderBackend::get_graphics_api())
            {
            case GraphicsAPI::OpenGL:
                glfwSwapBuffers(window.get_context().as<GLFWwindow>());
                break;
            default:
                CORE_ASSERT(false, u8"glfwWindow: Other API not supported!");
            }
            return false;
        });
        glfwPollEvents();
    }
}
