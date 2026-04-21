module;

#include <glad/glad.h>
#include <stb_image.h>

module Texture;
import :OpenGL;

import Log;
import FileUtils;

namespace {
    using namespace rke;

    struct glTextureFormat
    {
        GLenum internal_format;
        GLenum data_format;
        GLenum pixel_type;
        uint32 channels;
    };

    static inline glTextureFormat format_to_gl_enum(Texture::Format format)
    {
        switch(format)
        {
        case Texture::Format::RGBA16F:
            return { GL_RGBA16F, GL_RGBA, GL_FLOAT, 4 };
        case Texture::Format::RGBA8:
            return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4 };
        case Texture::Format::RGB8:
            return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, 3 };
        case Texture::Format::R8:
            return { GL_R8,	GL_RED, GL_UNSIGNED_BYTE, 1 };
        case Texture::Format::SRGB8_ALPHA8:
            return { GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, 4 };
        case Texture::Format::SRGB8:
            return { GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE, 3 };
        case Texture::Format::R32I:
            return { GL_R32I, GL_RED_INTEGER, GL_INT, 1 };
        case Texture::Format::DEPTH24_STENCIL8:
            return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 1 };
        default:
            CORE_ASSERT(false, u8"glTexture: Format no supported!");
            std::unreachable();
        }
    }

    static inline GLenum filt_format_to_gl_enum(Texture::FiltFormat filt_format)
    {
        switch(filt_format)
        {
        case Texture::FiltFormat::Linear:  return GL_LINEAR;
        case Texture::FiltFormat::Nearest: return GL_NEAREST;
        default:
            CORE_WARN(u8"glTexture: Unknown filt format! Using Linear.");
            return GL_LINEAR;
        }
    }

    static inline GLenum wrap_format_to_gl_enum(Texture::WrapFormat wrap_format)
    {
        switch(wrap_format)
        {
        case Texture::WrapFormat::Repeat:
            return GL_REPEAT;
        case Texture::WrapFormat::Clamp2Edge:
            return GL_CLAMP_TO_EDGE;
        default:
            CORE_WARN(u8"glTexture: Unknown wrap format! Using Clamp to Edge.");
            return GL_CLAMP_TO_EDGE;
        }
    }
}

namespace rke
{
    glTexture2D::glTexture2D(uint32 w, uint32 h, Format format, FiltFormat filt)
        : width_(w), height_(h), owns_texture_(true)
    {
        auto gl_format{ format_to_gl_enum(format) };
        internal_format_ = gl_format.internal_format;
        data_format_ = gl_format.data_format;
        pixel_type_	 = gl_format.pixel_type;
        channels_    = gl_format.channels;

        glCreateTextures(GL_TEXTURE_2D, 1, &renderer_id_);
        glTextureStorage2D(renderer_id_, 1, internal_format_, width_, height_);

        if(format == Format::DEPTH24_STENCIL8)
        {
            // Parameters for depth textures (e.g., shadow maps)
            glTextureParameteri(renderer_id_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(renderer_id_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTextureParameteri(renderer_id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(renderer_id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            float border_color[]{ 1.0f, 1.0f, 1.0f, 1.0f };
            glTextureParameterfv(renderer_id_, GL_TEXTURE_BORDER_COLOR, border_color);
        }
        else
        {
            // Parameters for color textures
            GLenum filtering{ filt_format_to_gl_enum(filt) };
            glTextureParameteri(renderer_id_, GL_TEXTURE_MIN_FILTER, filtering);
            glTextureParameteri(renderer_id_, GL_TEXTURE_MAG_FILTER, filtering);

            glTextureParameteri(renderer_id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(renderer_id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }

    glTexture2D::glTexture2D(const Path& filepath, FiltFormat filt, WrapFormat wrap, bool srgb)
        : owns_texture_(true)
    {
        int width{}, height{}, channels{};
        stbi_set_flip_vertically_on_load(1);

        Buffer texture_data{ file::read_file_binary(filepath) };
        if(texture_data.empty()) {
            CORE_ERROR(u8"glTexture: Texture '{}' not found or empty!", filepath);
            return;
        }
        stbi_uc* data{ stbi_load_from_memory(texture_data.data(),
            static_cast<int>(texture_data.size()), &width, &height, &channels, 0)};
        CORE_ASSERT(data, u8"glTexture: Failed to load image!");

        stbi_set_flip_vertically_on_load(0);
        width_  = width;
        height_ = height;

        pixel_type_ = GL_UNSIGNED_BYTE;
        channels_ = channels;
        switch(channels)
        {
        case 4:
            if(srgb) internal_format_ = GL_SRGB8_ALPHA8;
            else	 internal_format_ = GL_RGBA8;
            data_format_ = GL_RGBA;
            break;
        case 3:
            if(srgb) internal_format_ = GL_SRGB8;
            else	 internal_format_ = GL_RGB8;
            data_format_ = GL_RGB;
            break;
        case 1:
            internal_format_ = GL_R8;
            data_format_     = GL_RED;
            break;
        default:
            CORE_ASSERT(false, u8"glTexture: Format no supported!");
            break;
        }
        // GL_RGBA: 4 bytes(4 * sizeof(unsigned char))
        // GL_RGB : 3 bytes(3 * sizeof(unsigned char))...

        GLsizei levels{ 1 };
        if(filt == FiltFormat::Linear) {
            levels = static_cast<GLsizei>
                (std::floor(std::log2(std::max(width_, height_)))) + 1;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &renderer_id_);
        glTextureStorage2D(renderer_id_, levels, internal_format_, width_, height_);

        if(filt == FiltFormat::Linear) {
            glTextureParameteri(renderer_id_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(renderer_id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } else {
            glTextureParameteri(renderer_id_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(renderer_id_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        glTextureParameteri(renderer_id_, GL_TEXTURE_WRAP_S, wrap_format_to_gl_enum(wrap));
        glTextureParameteri(renderer_id_, GL_TEXTURE_WRAP_T, wrap_format_to_gl_enum(wrap));

        glTextureSubImage2D (
            renderer_id_, 0/*level*/, 0, 0, width_, height_,
            data_format_, GL_UNSIGNED_BYTE, data
        );
        if(levels > 1) glGenerateTextureMipmap(renderer_id_);

        stbi_image_free(data);
    }

    glTexture2D::glTexture2D(uint32 renderer_id, uint32 w, uint32 h, Format format)
        : renderer_id_(renderer_id), width_(w), height_(h), owns_texture_(false)
    {
        auto gl_format{ format_to_gl_enum(format) };
        internal_format_ = gl_format.internal_format;
        data_format_	 = gl_format.data_format;
        pixel_type_		 = gl_format.pixel_type;
        channels_		 = gl_format.channels;
    }

    glTexture2D::~glTexture2D() {
        if(owns_texture_) glDeleteTextures(1, &renderer_id_);
    }

    void glTexture2D::set_data(void* data, uint32 size)
    {
        glTextureSubImage2D (
            renderer_id_, 0, 0, 0, width_, height_,
            data_format_, pixel_type_, data
        );
    }

    void glTexture2D::bind(BindingPoint point) const
        { glBindTextureUnit(static_cast<uint32>(point), renderer_id_); }

    void glTexture2D::bind(uint32 slot) const
        { glBindTextureUnit(slot, renderer_id_); }
}
