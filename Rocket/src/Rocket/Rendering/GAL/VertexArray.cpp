module;
module VertexArray;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    Ref<VertexArray> VertexArray::create()
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glVertexArray>();
        default:
            CORE_ASSERT(false, u8"VertexArray: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
