module;

#include <glad/glad.h>

module RenderBackend;

import Log;

namespace rke
{
    void RenderBackend::set_proc_address_getter(void* getter)
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
        {
            int succeeded{ gladLoadGLLoader(reinterpret_cast<GLADloadproc>(getter)) };
            CORE_ASSERT(succeeded, u8"RenderBackend: Failed to initialize GLAD!");
            CORE_INFO(u8R"(OpenGL Context created
    -- OpenGL vendor  : {}
    -- OpenGL renderer: {}
    -- OpenGL version : {})",
            (const char*)glGetString(GL_VENDOR  ),
            (const char*)glGetString(GL_RENDERER),
            (const char*)glGetString(GL_VERSION));
        } break;
        default:
            CORE_ASSERT(false, u8"glfwWindow: Other API not supported!");
        }
    }
}
