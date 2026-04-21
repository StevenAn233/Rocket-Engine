module;

#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

module Context;
import :glfw;

import Log;
import RenderCommand;
import Instrumentor;
import String;

namespace rke
{
    glfwContext::glfwContext(NativeWindow handle)
        : handle_(handle.as<GLFWwindow>())
        { CORE_ASSERT(handle_, u8"RenderingContext: Window handle is null!"); }

    void glfwContext::init()
    {
        glfwMakeContextCurrent(handle_);
        int succeeded{ gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)) };
        CORE_ASSERT(succeeded, u8"glad: Failed to initialize GLAD!");

        CORE_INFO(u8R"(glContext: Context created
     -- OpenGL vendor  : {}
     -- OpenGL renderer: {}
     -- OpenGL version : {})",
            (const char*)glGetString(GL_VENDOR  ),
            (const char*)glGetString(GL_RENDERER),
            (const char*)glGetString(GL_VERSION));

        RenderCommand::enable_blend();
        RenderCommand::disable_srgb(); // manually applied in ToneMapping
        RenderCommand::enable_depth_test();
    }

    void glfwContext::swap_buffers()
    {
        RKE_PROFILE_FUNCTION();
        glfwSwapBuffers(handle_);
    }
}
