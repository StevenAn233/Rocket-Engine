module;

#include "rke_macros.h"

export module RenderBackend;

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
    };
}
