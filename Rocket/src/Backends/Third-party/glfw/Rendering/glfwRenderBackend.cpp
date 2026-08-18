module;

#include <glfw/glfw3.h>

module RenderBackend;

import Log;

namespace rke::render_backend::internal
{
    void* get_proc_address_getter()
    {
        switch(get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return reinterpret_cast<void*>(glfwGetProcAddress);
        default:
            CORE_ASSERT(false, u8"RenderBackend: Other APIs not supported!");
        }
        return nullptr;
    }
}
