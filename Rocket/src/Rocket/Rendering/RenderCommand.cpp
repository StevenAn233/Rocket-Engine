module;
module RenderCommand;
import :Base;
import :OpenGL;

import Log;
import HeapManager;
import RendererAPI;

namespace rke
{
    RenderCommand& RenderCommand::get_instance()
    {
        static Scope<RenderCommand> instance{ create() };
        return *instance;
    }

    Scope<RenderCommand> RenderCommand::create()
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: No graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_scope<glRenderCommand>();
    //  case GraphicAPI::Vulkan:
    //  case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: Unknown graphic api!");
            std::unreachable();
        }
    }
}
