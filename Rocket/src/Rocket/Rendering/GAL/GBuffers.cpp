module;
module GBuffers;
import :OpenGL;

import Log;
import HeapManager;
import RenderBackend;

namespace rke
{
    Ref<VertexBuffer> VertexBuffer::create(const void* data, uint32 size)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glVertexBuffer>(data, size);
        default:
            CORE_ASSERT(false, u8"VertexBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Ref<VertexBuffer> VertexBuffer::create(uint32 size)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glVertexBuffer>(size);
        default:
            CORE_ASSERT(false, u8"VertexBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Ref<IndexBuffer> IndexBuffer::create(const void* data, uint32 count)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glIndexBuffer>(data, count);
        default:
            CORE_ASSERT(false, u8"IndexBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Ref<UniformBuffer> UniformBuffer::create(uint32 size)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glUniformBuffer>(size);
        default:
            CORE_ASSERT(false, u8"UniformBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Ref<PixelBuffer> PixelBuffer::create(uint32 size)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glPixelBuffer>(size);
        default:
            CORE_ASSERT(false, u8"PixelBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }

    Ref<PixelBuffer> PixelBuffer::create(const void* data, uint32 size)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_ref<glPixelBuffer>(data, size);
        default:
            CORE_ASSERT(false, u8"PixelBuffer: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
