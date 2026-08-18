module;

#include "rke_macros.h"

export module RenderBackend;

import NativeWindow;

export namespace rke
{
    enum class GraphicsAPI
    {
        None    = 0,
        OpenGL  = 1,
        Vulkan  = 2,
        DirectX = 3
    };
}

export namespace rke::render_backend
{
    RKE_API GraphicsAPI get_graphics_api();
    RKE_API void init_window_context(NativeWindow context);
}

namespace rke::render_backend::internal
{
    void* get_proc_address_getter();
    void set_proc_address_getter(void* getter);
}
