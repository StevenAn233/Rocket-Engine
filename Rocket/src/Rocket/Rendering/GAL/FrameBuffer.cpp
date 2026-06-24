module;
module FrameBuffer;
import :OpenGL;

import RendererAPI;
import HeapManager;
import Log;

namespace rke
{
    Ref<FrameBuffer> FrameBuffer::create(FrameBuffer::Specification spec)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glFrameBuffer>(spec);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }
}
