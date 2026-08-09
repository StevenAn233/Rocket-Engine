module;
module Texture;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    Scope<Texture2D> Texture2D::create(uint32 w, uint32 h, Format format, FiltFormat filt)
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glTexture2D>(w, h, format, filt);
        default:
            CORE_ASSERT(false, u8"Texture2D: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Scope<Texture2D> Texture2D::create(const Path& filepath,
        FiltFormat filt, WrapFormat wrap, bool srgb)
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glTexture2D>(filepath, filt, wrap, srgb);
        default:
            CORE_ASSERT(false, u8"Texture2D: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Scope<Texture2D> Texture2D::create_from_id
    (uint32 renderer_id, uint32 w, uint32 h, Format format)
    {
        switch(RenderBackend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glTexture2D>(renderer_id, w, h, format);
        default:
            CORE_ASSERT(false, u8"Texture2D: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
