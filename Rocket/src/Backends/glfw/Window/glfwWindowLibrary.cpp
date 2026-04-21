module;

#include <glad/glad.h>
#include <glfw/glfw3.h>

module WindowLibrary;
import :glfw;

import Log;

namespace {
    static void error_callback(int error, const char* description)
        { CORE_ERROR(u8"GLFW: ERROR({}), {}!", error, description); }
}

namespace rke
{
    glfwWindowLibrary::glfwWindowLibrary()
    {
        int is_succeeded{ glfwInit() };
        CORE_ASSERT(is_succeeded, u8"glfw: Failed to initialize GLFW!");
        glfwSetErrorCallback(error_callback);

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        master_context_ = glfwCreateWindow(1, 1, "Hidden Main Context", nullptr, nullptr);
        CORE_ASSERT(master_context_,
            u8"glfwWindowLibrary: Failed to create_ref hidden main context window!");
        glfwMakeContextCurrent(master_context_);
        int succeeded{ gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)) };
        CORE_ASSERT(succeeded, u8"glad: Failed to initialize GLAD!");
    }

    glfwWindowLibrary::~glfwWindowLibrary()
    {
        windows_.clear();
        if(master_context_) glfwDestroyWindow(master_context_);
        glfwTerminate();
        CORE_INFO(u8"glfwWindowLibrary: GLFW terminated.");
    }

    void glfwWindowLibrary::refresh()
    {
        std::erase_if(windows_, [](auto& pair)
        {
            auto& [_, window]{ pair };
            if(window->should_close()) return true;
            window->swap_buffers();
            return false;
        });
        glfwPollEvents();
    }

    void glfwWindowLibrary::make_master_context_current()
        { glfwMakeContextCurrent(master_context_); }

    void glfwWindowLibrary::add(Scope<Window> window)
    {
        const String& name{ window->get_title() };
        CORE_ASSERT(!exists(name), u8"WindowLibrary: Window name already exists!");
        windows_[name] = std::move(window);
    }

    void glfwWindowLibrary::load(const Window::WindowProps& props)
    {
        Scope<Window> window{ Window::
            create(props, NativeWindow(master_context_))};
        add(std::move(window));
    }

    Window* glfwWindowLibrary::operator[](const String& name)
    {
        CORE_ASSERT(exists(name), u8"WindowLibrary: Window '{}' not found!", name);
        return windows_.at(name).get(); // not const
    }

    const Window* glfwWindowLibrary::operator[](const String& name) const
    {
        CORE_ASSERT(exists(name), u8"WindowLibrary: Window '{}' not found!", name);
        return windows_.at(name).get(); // const
    }
}
