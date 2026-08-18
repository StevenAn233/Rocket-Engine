module;
module VertexArray;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    Scope<VertexArray> VertexArray::create()
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glVertexArray>();
        default:
            CORE_ASSERT(false, u8"VertexArray: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
