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

    class RKE_API RenderBackend
    {
    public:
        static GraphicsAPI get_graphics_api();
        static void init_window_context(NativeWindow context);
    private:
        static void* get_proc_address_getter();
        static void set_proc_address_getter(void* getter);
    };
}
