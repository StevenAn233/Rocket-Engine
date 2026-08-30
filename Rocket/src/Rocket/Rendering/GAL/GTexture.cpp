module;
module GTexture;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    Scope<GTexture2D> GTexture2D::create(uint32 w, uint32 h,
        Format format, const void* data, FiltFormat filt, WrapFormat wrap)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glGTexture2D>(w, h, format, data, filt, wrap);
        default:
            CORE_ASSERT(false, u8"GTexture2D: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Scope<GTexture2D> GTexture2D::create_from_id
    (uint32 gal_id, uint32 w, uint32 h, Format format)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glGTexture2D>(gal_id, w, h, format);
        default:
            CORE_ASSERT(false, u8"GTexture2D: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
