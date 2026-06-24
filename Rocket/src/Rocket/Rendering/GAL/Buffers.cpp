module;
module Buffers;
import :OpenGL;

import RendererAPI;
import Log;

namespace rke
{
    Ref<VertexBuffer> VertexBuffer::create(const void* data, uint32 size)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glVertexBuffer>(data, size);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<VertexBuffer> VertexBuffer::create(uint32 size)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glVertexBuffer>(size);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<IndexBuffer> IndexBuffer::create(const void* data, uint32 count)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glIndexBuffer>(data, count);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<UniformBuffer> UniformBuffer::create(uint32 size)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glUniformBuffer>(size);
        //case GraphicAPI::Vulkan:
        //case GraphicAPI::DirectX:
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<PixelBuffer> PixelBuffer::create(uint32 size)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glPixelBuffer>(size);
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }

    Ref<PixelBuffer> PixelBuffer::create(const void* data, uint32 size)
    {
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glPixelBuffer>(data, size);
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
    }
}
