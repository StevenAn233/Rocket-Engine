module;

#include <glad/glad.h>
#include <glfw/glfw3.h>

module WindowsLib;
import :glfw;

import Log;

namespace {
    static void error_callback(int error, const char* description)
        { CORE_ERROR(u8"GLFW: ERROR({}), {}!", error, description); }
}

namespace rke
{
    glfwWindowsLib::glfwWindowsLib()
    {
        CORE_ASSERT(glfwInit(), u8"glfw: Failed to initialize GLFW!");
        glfwSetErrorCallback(error_callback);

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        master_context_ = glfwCreateWindow(1, 1, "Hidden Main Context", nullptr, nullptr);
        CORE_ASSERT(master_context_, u8"glfwWindowsLib: Failed to create master context!");
        glfwMakeContextCurrent(reinterpret_cast<GLFWwindow*>(master_context_));

        int succeeded{ gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) };
        CORE_ASSERT(succeeded, u8"glad: Failed to initialize GLAD!"); // ??? why here?
    }

    glfwWindowsLib::~glfwWindowsLib()
    {
        map_.clear();
        if(master_context_) glfwDestroyWindow
            (reinterpret_cast<GLFWwindow*>(master_context_));
        glfwTerminate();
        CORE_INFO(u8"glfwWindowsLib: GLFW terminated.");
    }

    void glfwWindowsLib::refresh()
    {
        std::erase_if(map_, [](auto& pair)
        {
            auto& [_, window]{ pair };
            if(window->should_close()) return true;
            window->swap_buffers();
            return false;
        });
        glfwPollEvents();
    }

    Window* glfwWindowsLib::load(Scope<Window::Props> props)
    {
        Scope<Window> window{ Window::create
            (std::move(props), NativeWindow(master_context_)) };
        return add(std::move(window));
    }

    NativeWindow glfwWindowsLib::get_current_context() const
        { return NativeWindow(glfwGetCurrentContext()); }
    NativeWindow glfwWindowsLib::get_master_context() const
        { return NativeWindow(master_context_); }

    void glfwWindowsLib::make_master_context_current()
    {
        glfwMakeContextCurrent
            (reinterpret_cast<GLFWwindow*>(master_context_));
    }
}
