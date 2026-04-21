module;

#include "rke_macros.h"

export module RendererAPI;

export namespace rke
{
    class RKE_API RendererAPI
    {
    public:
        enum class GraphicAPI
        {
            None    = 0,
            OpenGL  = 1,
            Vulkan  = 2,
            DirectX = 3
        };

        static GraphicAPI get_graphic_api();
    };
}
