module;
module VertexArray;
import :OpenGL;

import HeapManager;
import Log;
import RendererAPI;

namespace rke
{
    Ref<VertexArray> VertexArray::create()
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glVertexArray>();
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }
}
