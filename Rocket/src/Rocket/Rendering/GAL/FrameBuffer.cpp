module;
module FrameBuffer;
import :OpenGL;

import RenderBackend;
import HeapManager;
import Log;

namespace rke
{
    Scope<FrameBuffer> FrameBuffer::create(FrameBuffer::Specification spec)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glFrameBuffer>(spec);
        default:
            CORE_ASSERT(false, u8"FrameBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
