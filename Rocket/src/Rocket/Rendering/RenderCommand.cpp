module;
module RenderCommand;
import :Base;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    Scope<RenderCommand> RenderCommand::create()
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glRenderCommand>();
        default:
            CORE_ASSERT(false, u8"RenderCommand: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
