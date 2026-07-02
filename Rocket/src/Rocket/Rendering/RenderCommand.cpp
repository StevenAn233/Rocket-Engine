module;
module RenderCommand;
import :Base;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    RenderCommand& RenderCommand::get_instance()
    {
        static Scope<RenderCommand> instance{ create() };
        return *instance;
    }

    Scope<RenderCommand> RenderCommand::create()
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glRenderCommand>();
        default:
            CORE_ASSERT(false, u8"RenderCommand: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
