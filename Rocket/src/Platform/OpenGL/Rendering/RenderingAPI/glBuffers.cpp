module;

#include <glad/glad.h>

module Buffers;
import :OpenGL;

import Log;

namespace {
    using namespace rke;

    static inline GLenum usage_to_gl_target(PixelBuffer::Usage usage)
    {
        switch (usage)
        {
        case PixelBuffer::Usage::Pack:   return GL_PIXEL_PACK_BUFFER;
        case PixelBuffer::Usage::Unpack: return GL_PIXEL_UNPACK_BUFFER;
        }
        CORE_ASSERT(false, u8"glBuffers: Unknown PixelBufferUsage!");
        return 0;
    }

    static inline GLenum get_gl_access(GBuffer::Access access)
    {
        switch (access)
        {
        case rke::GBuffer::Access::Read:
            return GL_READ_ONLY;
        case rke::GBuffer::Access::Write:
            return GL_WRITE_ONLY;
        case rke::GBuffer::Access::ReadWrite:
            return GL_READ_WRITE;
        }
        CORE_ASSERT(false, u8"glBuffers: Unknown GBuffer Access!");
        return 0;
    }

    static constexpr GLbitfield GL_MAP_ALL_ACCESS
        { GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT };
    // GL_DYNAMIC_STORAGE_BIT means data can be changed
}

namespace rke
{
// glVertexBuffer
    glVertexBuffer::glVertexBuffer(const void* data, uint32 size)
    {
        glCreateBuffers(1, &renderer_id_);
        // requires version 4.5+, directly arrange memory while generating id
        glNamedBufferStorage(renderer_id_, size, data, GL_MAP_ALL_ACCESS);
    }
    glVertexBuffer::glVertexBuffer(uint32 size)
    {
        glCreateBuffers(1, &renderer_id_);
        glNamedBufferStorage(renderer_id_, size, nullptr, GL_MAP_ALL_ACCESS);
    }
    glVertexBuffer::~glVertexBuffer() { glDeleteBuffers(1, &renderer_id_); }

    void glVertexBuffer::set_data(const void* new_data, uint32 new_size)
        { glNamedBufferSubData(renderer_id_, 0, new_size, new_data); }

    void* glVertexBuffer::map(Access access)
        { return glMapNamedBuffer(renderer_id_, get_gl_access(access)); }

    void glVertexBuffer::unmap() { glUnmapNamedBuffer(renderer_id_); }

// glIndexBuffer
    glIndexBuffer::glIndexBuffer(const void* data, uint32 count) : count_(count)
    {
        glCreateBuffers(1, &renderer_id_);
        glNamedBufferStorage(renderer_id_,
            count * sizeof(uint32), data, GL_MAP_ALL_ACCESS);
    }
    glIndexBuffer::~glIndexBuffer() { glDeleteBuffers(1, &renderer_id_); }

    void glIndexBuffer::set_data(const void* new_data, uint32 new_count)
        { glNamedBufferSubData(renderer_id_, 0, new_count * sizeof(uint32), new_data); }

    void* glIndexBuffer::map(Access access)
        { return glMapNamedBuffer(renderer_id_, get_gl_access(access)); }

    void glIndexBuffer::unmap() { glUnmapNamedBuffer(renderer_id_); }

// glUniformBuffer
    glUniformBuffer::glUniformBuffer(uint32 size)
    {
        glCreateBuffers(1, &renderer_id_);
        glNamedBufferStorage(renderer_id_, size, nullptr, GL_MAP_ALL_ACCESS);
    }
    glUniformBuffer::~glUniformBuffer() { glDeleteBuffers(1, &renderer_id_); }

    void glUniformBuffer::set_data(const void* data, uint32 size, uint32 offset)
        { glNamedBufferSubData(renderer_id_, offset, size, data); }

    void glUniformBuffer::bind(BindingPoint point)
        { glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<uint32>(point), renderer_id_); }

    void* glUniformBuffer::map(Access access)
        { return glMapNamedBuffer(renderer_id_, get_gl_access(access)); }

    void glUniformBuffer::unmap() { glUnmapNamedBuffer(renderer_id_); }

// glPixelBuffer
    glPixelBuffer::glPixelBuffer(uint32 size)
    {
        glCreateBuffers(1, &renderer_id_);
        glNamedBufferStorage(renderer_id_, size, nullptr, GL_MAP_ALL_ACCESS);
    }
    glPixelBuffer::glPixelBuffer(const void* data, uint32 size)
    {
        glCreateBuffers(1, &renderer_id_);
        glNamedBufferStorage(renderer_id_, size, data, GL_MAP_ALL_ACCESS);
    }
    glPixelBuffer::~glPixelBuffer() { glDeleteBuffers(1, &renderer_id_); }

    void glPixelBuffer::set_data(const void* data, uint32 size)
        { glNamedBufferSubData(renderer_id_, 0, size, data); }

    void glPixelBuffer::bind(PixelBuffer::Usage usage)
        { glBindBuffer(usage_to_gl_target(usage), renderer_id_); }

    void glPixelBuffer::unbind(PixelBuffer::Usage usage)
        { glBindBuffer(usage_to_gl_target(usage), 0); }

    void* glPixelBuffer::map(Access access)
        { return glMapNamedBuffer(renderer_id_, get_gl_access(access)); }

    void glPixelBuffer::unmap() { glUnmapNamedBuffer(renderer_id_); }
}
