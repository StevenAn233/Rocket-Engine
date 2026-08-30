module;

#include <glad/glad.h>
#include <cmath>
#include <algorithm>

module GTexture;
import :OpenGL;

import Log;

namespace {
    using namespace rke;

    struct glTextureFormat
    {
        GLenum internal_format;
        GLenum data_format;
        GLenum pixel_type;
        uint32 channels;
    };

    static inline glTextureFormat format_to_gl_enum(GTexture::Format format)
    {
        switch(format)
        {
        case GTexture::Format::RGBA16F:
            return { GL_RGBA16F, GL_RGBA, GL_FLOAT, 4 };
        case GTexture::Format::RGBA8:
            return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4 };
        case GTexture::Format::RGB8:
            return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, 3 };
        case GTexture::Format::R8:
            return { GL_R8,	GL_RED, GL_UNSIGNED_BYTE, 1 };
        case GTexture::Format::SRGB8_ALPHA8:
            return { GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, 4 };
        case GTexture::Format::SRGB8:
            return { GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE, 3 };
        case GTexture::Format::R32I:
            return { GL_R32I, GL_RED_INTEGER, GL_INT, 1 };
        case GTexture::Format::DEPTH24_STENCIL8:
            return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 1 };
        default:
            CORE_ASSERT(false, u8"glGTexture: Format no supported!");
            std::unreachable();
        }
    }

    static inline GLenum filt_format_to_gl_enum(GTexture::FiltFormat filt_format)
    {
        switch(filt_format)
        {
        case GTexture::FiltFormat::Linear:  return GL_LINEAR;
        case GTexture::FiltFormat::Nearest: return GL_NEAREST;
        default:
            CORE_WARN(u8"glGTexture: Unknown filt format! Using Linear.");
            return GL_LINEAR;
        }
    }

    static inline GLenum wrap_format_to_gl_enum(GTexture::WrapFormat wrap_format)
    {
        switch(wrap_format)
        {
        case GTexture::WrapFormat::Repeat:
            return GL_REPEAT;
        case GTexture::WrapFormat::Clamp2Edge:
            return GL_CLAMP_TO_EDGE;
        default:
            CORE_WARN(u8"glGTexture: Unknown wrap format! Using Clamp to Edge.");
            return GL_CLAMP_TO_EDGE;
        }
    }
}

namespace rke
{
    glGTexture2D::glGTexture2D(uint32 w, uint32 h, Format format,
        const void* data, FiltFormat filt, WrapFormat wrap)
        : width_(w), height_(h), owns_texture_(true)
    {
        auto gl_format{ format_to_gl_enum(format) };
        internal_format_ = gl_format.internal_format;
        data_format_ = gl_format.data_format;
        pixel_type_	= gl_format.pixel_type;
        channels_ = gl_format.channels;

        if(format == Format::DEPTH24_STENCIL8)
        {
            // depth textures are created empty
            glCreateTextures(GL_TEXTURE_2D, 1, &gal_id_);
            glTextureStorage2D(gal_id_, 1, internal_format_, width_, height_);

            glTextureParameteri(gal_id_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(gal_id_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTextureParameteri(gal_id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(gal_id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            float border_color[]{ 1.0f, 1.0f, 1.0f, 1.0f };
            glTextureParameterfv(gal_id_, GL_TEXTURE_BORDER_COLOR, border_color);
            return;
        }

        GLsizei levels{ 1 };
        if(data && filt == FiltFormat::Linear)
            levels += static_cast<GLsizei>(std::floor(std::log2(std::max(width_, height_))));

        glCreateTextures(GL_TEXTURE_2D, 1, &gal_id_);
        glTextureStorage2D(gal_id_, levels, internal_format_, width_, height_);

        if(data && filt == FiltFormat::Linear) {
            glTextureParameteri(gal_id_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(gal_id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLfloat max_aniso{};
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
            glTextureParameterf(gal_id_, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
        } else {
            GLenum filtering{ filt_format_to_gl_enum(filt) };
            glTextureParameteri(gal_id_, GL_TEXTURE_MIN_FILTER, filtering);
            glTextureParameteri(gal_id_, GL_TEXTURE_MAG_FILTER, filtering);
        }

        GLenum wrapping{ wrap_format_to_gl_enum(wrap) };
        glTextureParameteri(gal_id_, GL_TEXTURE_WRAP_S, wrapping);
        glTextureParameteri(gal_id_, GL_TEXTURE_WRAP_T, wrapping);

        if(data) {
            glTextureSubImage2D(gal_id_,
                0, 0, 0, width_, height_,
                data_format_, pixel_type_, data);
            if(levels > 1) glGenerateTextureMipmap(gal_id_);
        }
    }

    glGTexture2D::glGTexture2D(uint32 gal_id, uint32 w, uint32 h, Format format)
        : gal_id_(gal_id), width_(w), height_(h), owns_texture_(false)
    {
        auto gl_format{ format_to_gl_enum(format) };
        internal_format_ = gl_format.internal_format;
        data_format_ = gl_format.data_format;
        pixel_type_	= gl_format.pixel_type;
        channels_ = gl_format.channels;
    }

    glGTexture2D::~glGTexture2D()
        { if(owns_texture_) glDeleteTextures(1, &gal_id_); }

    void glGTexture2D::set_data(void* data, uint32 size)
    {
        glTextureSubImage2D(gal_id_,
            0, 0, 0, width_, height_,
            data_format_, pixel_type_, data);
    }

    void glGTexture2D::bind(BindingPoint point) const
        { glBindTextureUnit(static_cast<uint32>(point), gal_id_); }

    void glGTexture2D::bind(uint32 slot) const
        { glBindTextureUnit(slot, gal_id_); }
}
