module;
module Texture;
import :OpenGL;

import Log;
import HeapManager;
import RendererAPI;

namespace rke
{
    Ref<Texture2D> Texture2D::create(uint32 w, uint32 h, Format format, FiltFormat filt)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glTexture2D>(w, h, format, filt);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<Texture2D> Texture2D::create(const Path& filepath,
        FiltFormat filt, WrapFormat wrap, bool srgb)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glTexture2D>(filepath, filt, wrap, srgb);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<Texture2D> Texture2D::create_from_id
    (uint32 renderer_id, uint32 w, uint32 h, Format format)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glTexture2D>(renderer_id, w, h, format);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }
}
