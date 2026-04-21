module;

#include <glad/glad.h>

module FrameBuffer;

import :OpenGL;

import Log;
import RenderCommand;

namespace {
    using namespace rke;

    constexpr uint32 MAX_FRAME_BUFFER_SIZE{ 8192u };

    static inline bool is_depth_format(Texture::Format format)
    {
        switch(format)
        {
        case Texture::Format::DEPTH24_STENCIL8:
            return true;
        // other cases...
        }
        return false;
    }

    static inline GLenum texture_type(bool multi_sampled)
        { return multi_sampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }

    static inline void create_textures(bool multi_sampled, uint32* out_id, uint32 count = 1)
    {
        if(multi_sampled) glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, count, out_id);
        else glCreateTextures(GL_TEXTURE_2D, count, out_id);
    }

    static inline void attach_msaa_color_texture(uint32 renderer_id,
        Size index, uint32& color_attachment, int samples,
        GLenum internal_format, uint32 w, uint32 h)
    {
        glTextureStorage2DMultisample(color_attachment, samples, internal_format, w, h, GL_TRUE);
        glNamedFramebufferTexture(renderer_id,
            static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + index), color_attachment, 0);
    }

    static void attach_color_texture(uint32 renderer_id,
        Size index, uint32& color_attachment,
        GLenum internal_format, GLenum filtering_format, uint32 w, uint32 h)
    {
        glTextureStorage2D(color_attachment, 1, internal_format, w, h);
        glNamedFramebufferTexture(renderer_id, GL_COLOR_ATTACHMENT0 + index, color_attachment, 0);

        glTextureParameteri(color_attachment, GL_TEXTURE_MIN_FILTER, filtering_format);
        glTextureParameteri(color_attachment, GL_TEXTURE_MAG_FILTER, filtering_format);

        glTextureParameteri(color_attachment, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); // hard-coded
        glTextureParameteri(color_attachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // hard-coded
        glTextureParameteri(color_attachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // hard-coded
    }

    static inline void attach_msaa_depth_texture(uint32 renderer_id,
        uint32& depth_attachment_id, int samples,
        GLenum internal_format, GLenum attachement_type, uint32 w, uint32 h)
    {
        glTextureStorage2DMultisample(depth_attachment_id, samples, internal_format, w, h, GL_TRUE);
        glNamedFramebufferTexture(renderer_id, attachement_type, depth_attachment_id, 0);
    }

    static inline void attach_depth_texture(uint32 renderer_id, uint32& depth_attachment_id,
        GLenum internal_format, GLenum attachement_type, uint32 w, uint32 h)
    {
        glTextureStorage2D(depth_attachment_id, 1, internal_format, w, h);
        glNamedFramebufferTexture(renderer_id, attachement_type, depth_attachment_id, 0);
    }

    static inline GLenum tex_format_to_gl_enum(Texture::Format fb_format)
    {
        switch (fb_format)
        {
        case Texture::Format::RGBA8:   return GL_RGBA8;
        case Texture::Format::RGB8:    return GL_RGB8;
        case Texture::Format::R8:      return GL_R8;
        case Texture::Format::R32I:    return GL_R32I;
        case Texture::Format::RGBA16F: return GL_RGBA16F;
        case Texture::Format::SRGB8:   return GL_SRGB8;
        case Texture::Format::SRGB8_ALPHA8:     return GL_SRGB8_ALPHA8;
        case Texture::Format::DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
        default:
            CORE_ASSERT(false, u8"glFrameBuffer: Unknown format conversion!");
            return GL_SRGB8_ALPHA8;
        }
    }

    static inline GLenum filt_format_to_gl_enum(Texture::FiltFormat filt_format)
    {
        switch(filt_format)
        {
        case Texture::FiltFormat::Linear:  return GL_LINEAR;
        case Texture::FiltFormat::Nearest: return GL_NEAREST;
        default:
            CORE_ASSERT(false, u8"glFrameBuffer: Unknown format conversion!");
            return GL_LINEAR;
        }
    }
}

namespace rke
{
    glFrameBuffer::glFrameBuffer(Specification spec) : spec_(std::move(spec))
    {
        GLint max_samples{};
        glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
        if(spec_.samples > max_samples)
        {
            CORE_WARN(u8"glFrameBuffer: GPU only supports up to {0} samples. "
                u8"Samples set to {0}.", max_samples);
            spec_.samples = max_samples;
        }
        invalidate();
    }

    glFrameBuffer::~glFrameBuffer()
    {
        glDeleteFramebuffers(1, &renderer_id_);
        glDeleteTextures(attachments_.size(), attachments_.data());

        renderer_id_ = 0;
        attachments_.clear();
        output_textures_.clear();

        if(msaa_renderer_id_)
        {
            glDeleteFramebuffers(1, &msaa_renderer_id_);
            glDeleteTextures(msaa_attachments_.size(), msaa_attachments_.data());

            msaa_renderer_id_ = 0;
            msaa_attachments_.clear();
        }
    }

    void glFrameBuffer::clear()
    {
        auto fbo{ spec_.samples > 1 ? msaa_renderer_id_ : renderer_id_ };
        auto& attachment_specs{ spec_.attachment_spec.texture_specs };
        int color_attachment_index{};
        for(Size i{}; i < attachment_specs.size(); i++)
        {
            if(attachment_specs[i].load_op == TextureSpecification::LoadOp::LOAD) continue;
            switch(attachment_specs[i].format)
            {
            case Texture::Format::RGBA8:
            case Texture::Format::RGBA16F:
            case Texture::Format::SRGB8_ALPHA8:
            {
                const glm::vec4* val{ std::get_if<glm::vec4>(&attachment_specs[i].clear_value) };
                CORE_ASSERT(val, u8"glFrameBuffer: Clear value type doesn't match with format!");
                RenderCommand::clear_color_buffer(fbo, color_attachment_index, *val);
                color_attachment_index++; 
            } break;
            case Texture::Format::RGB8:
            case Texture::Format::SRGB8:
            {
                const glm::vec3* val{ std::get_if<glm::vec3>(&attachment_specs[i].clear_value) };
                CORE_ASSERT(val, u8"glFrameBuffer: Clear value type doesn't match with format!");
                RenderCommand::clear_color_buffer(fbo, color_attachment_index, *val);
                color_attachment_index++;
            } break;
            case Texture::Format::R32I:
            {
                const int* val{ std::get_if<int>(&attachment_specs[i].clear_value) };
                CORE_ASSERT(val, u8"glFrameBuffer: Clear value type doesn't match with format!");
                RenderCommand::clear_color_buffer(fbo, color_attachment_index, *val);
                color_attachment_index++;
            } break;
            case Texture::Format::R8:
            {
                const float* val{ std::get_if<float>(&attachment_specs[i].clear_value) };
                CORE_ASSERT(val, u8"glFrameBuffer: Clear value type doesn't match with format!");
                RenderCommand::clear_color_buffer(fbo, color_attachment_index, *val);
                color_attachment_index++;
            } break;
            case Texture::Format::DEPTH24_STENCIL8:
                RenderCommand::clear_depth_buffer(fbo, 1.0f, 0);
                break;
            default: CORE_ASSERT(false, u8"glFrameBuffer: Unsupported texture format!");
            }
        }
    }

    uint32 glFrameBuffer::get_attachment(int index) const
    {
        if(zero_sized() || over_sized()) return 0;
        CORE_ASSERT(index < attachments_.size(),
            u8"glFrameBuffer: Try to access non-valid color attachment!");
        return attachments_[index];
    }
    const Texture2D* glFrameBuffer::get_texture(int index) const
    {
        if(zero_sized() || over_sized()) return nullptr;
        CORE_ASSERT(index < attachments_.size(),
            u8"glFrameBuffer: Try to access non-valid texture!");
        return output_textures_[index].get();
    }

    void glFrameBuffer::resize(uint32 w, uint32 h)
    {
        spec_.width  = w;
        spec_.height = h;

        invalidate();
    }

    void glFrameBuffer::set_samples(uint32 samples)
    {
        if(spec_.samples == samples) return;

        GLint max_samples{};
        glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

        uint32 new_samples{ samples };
        if(new_samples > max_samples)
        {
            CORE_WARN(u8"glFrameBuffer: GPU only supports up to {0} samples. "
                u8"Samples set to {0}.", max_samples);
            new_samples = max_samples;
        }
        if(spec_.samples == new_samples) return;
        spec_.samples = new_samples;
        invalidate();
    }

    int glFrameBuffer::read_pixel(uint32 color_attach_index, int x, int y)
    {
        CORE_ASSERT(color_attach_index < attachments_.size(),
            u8"glFrameBuffer: Try to access invalid color attachment!");

        if(!pixel_pbo_) pixel_pbo_ = PixelBuffer::create(sizeof(int));

        if(pbo_has_pending_request_)
        {
            int* ptr{ reinterpret_cast<int*>(pixel_pbo_->map(GBuffer::Access::Read)) };
            if(ptr) {
                last_read_pixel_value_ = *ptr;
                pixel_pbo_->unmap();
            }
        
            pbo_has_pending_request_ = false;
        }

        if(x >= 0 && y >= 0 && x < spec_.width && y < spec_.height)
        {
            GLint previous_read_fbo{};
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read_fbo);
        
            glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer_id_);
            glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attach_index);

            pixel_pbo_->bind(PixelBuffer::Usage::Pack);
            glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, nullptr);
            pixel_pbo_->unbind(PixelBuffer::Usage::Pack);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, previous_read_fbo);

            pbo_has_pending_request_ = true;
        }

        return last_read_pixel_value_;
    }

    void glFrameBuffer::clear_pbo()
    {
        if(!pixel_pbo_) return;
        auto data{ reinterpret_cast<GLubyte*>
            (pixel_pbo_->map(PixelBuffer::Access::Write)) };
        if(data) { memset(data, 0, 4); pixel_pbo_->unmap(); }
    }

    bool glFrameBuffer::zero_sized() const
        { return spec_.width == 0u || spec_.height == 0u; }

    bool glFrameBuffer::over_sized() const
    {
        return (spec_.width  > MAX_FRAME_BUFFER_SIZE)
            || (spec_.height > MAX_FRAME_BUFFER_SIZE);
    }

    void glFrameBuffer::bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER,
            spec_.samples > 1 ? msaa_renderer_id_ : renderer_id_);
        glViewport(0, 0, spec_.width, spec_.height);
        clear();
    }

    void glFrameBuffer::unbind()
    {
        // resolve the data of msaa_fbo into regular_fbo
        if(spec_.samples > 1)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_renderer_id_);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer_id_);

            Size color_attachment_count{};
            bool has_depth{ false };
            for(const auto& spec : spec_.attachment_spec.texture_specs)
            {
                if(is_depth_format(spec.format)) has_depth = true;
                else color_attachment_count++;
            }
            for(Size i{}; i < color_attachment_count; i++)
            {
                glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
                glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
                glBlitFramebuffer(0, 0, spec_.width, spec_.height, 0, 0, spec_.width, spec_.height,
                                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
            if(has_depth) // Blit depth buffer
            {
                glBlitFramebuffer(0, 0, spec_.width, spec_.height, 0, 0, spec_.width, spec_.height,
                                  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void glFrameBuffer::invalidate()
    {
    // clean-up
        if(renderer_id_) {
            glDeleteFramebuffers(1, &renderer_id_);
            glDeleteTextures(attachments_.size(), attachments_.data());

            renderer_id_ = 0;
            attachments_.clear();
            output_textures_.clear();
        }
        if(msaa_renderer_id_) {
            glDeleteFramebuffers(1, &msaa_renderer_id_);
            glDeleteTextures(msaa_attachments_.size(), msaa_attachments_.data());

            msaa_renderer_id_ = 0;
            msaa_attachments_.clear();
        }

        bool multi_sampled{ spec_.samples > 1 };

    // re-create
        glCreateFramebuffers(1, &renderer_id_);
        if(multi_sampled) glCreateFramebuffers(1, &msaa_renderer_id_);

        if(zero_sized() || over_sized()) {
            glNamedFramebufferDrawBuffer(renderer_id_, GL_NONE);
            if(multi_sampled) glNamedFramebufferDrawBuffer(msaa_renderer_id_, GL_NONE);
            return;
        }

        auto& attachment_specs{ spec_.attachment_spec.texture_specs };
        attachments_.resize(attachment_specs.size());
        create_textures(false, attachments_.data(), attachments_.size());
        if(multi_sampled) {
            msaa_attachments_.resize(attachment_specs.size());
            create_textures(true, msaa_attachments_.data(), msaa_attachments_.size());
        }

      // attach textures
        int color_attachment_index{};
        for(Size i{}; i < attachment_specs.size(); i++)
        {
            if(is_depth_format(attachment_specs[i].format)) {
                switch(attachment_specs[i].format)
                {
                case Texture::Format::DEPTH24_STENCIL8:
                    attach_depth_texture(renderer_id_, attachments_[i],
                        GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT,
                        spec_.width, spec_.height);
                    if(multi_sampled) attach_msaa_depth_texture(msaa_renderer_id_,
                        msaa_attachments_[i], spec_.samples,
                        GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT,
                        spec_.width, spec_.height);
                    break;
                default:
                    CORE_ASSERT(false, u8"glFrameBuffer: Undefined texture format!");
                    std::unreachable();
                }
            } else {
                GLenum format{ tex_format_to_gl_enum(attachment_specs[i].format) };
                attach_color_texture(renderer_id_, color_attachment_index, attachments_[i], format,
                    filt_format_to_gl_enum(attachment_specs[i].filtering), spec_.width, spec_.height);
                if(multi_sampled) attach_msaa_color_texture(msaa_renderer_id_,
                    color_attachment_index, msaa_attachments_[i],
                    spec_.samples, format, spec_.width, spec_.height);
                color_attachment_index++;
            }
        }

      // draw buffers setting
        if(color_attachment_index > 0) {
            std::vector<GLenum> buffers{};
            for(Size i{}; i < color_attachment_index; i++)
                buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
            glNamedFramebufferDrawBuffers(renderer_id_, buffers.size(), buffers.data());
            if(multi_sampled) glNamedFramebufferDrawBuffers
                (msaa_renderer_id_, buffers.size(), buffers.data());
        } else { // Depth-only framebuffer
            glNamedFramebufferDrawBuffer(renderer_id_, GL_NONE);
            if(multi_sampled) glNamedFramebufferDrawBuffer(msaa_renderer_id_, GL_NONE);
        }

        // check validation
        CORE_ASSERT(glCheckNamedFramebufferStatus
            (renderer_id_, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            u8"glFrameBuffer: Frame buffer is not complete!");
        if(multi_sampled) {
            CORE_ASSERT(glCheckNamedFramebufferStatus
                (msaa_renderer_id_, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                u8"glFrameBuffer: Msaa Frame buffer is not complete!");
        }

        // create Texture2D(s)
        output_textures_.resize(attachment_specs.size());
        for(Size i{}; i < output_textures_.size(); i++) {
            output_textures_[i] = Texture2D::create_from_id
                (attachments_[i], spec_.width, spec_.height, attachment_specs[i].format);
        }
    }
}
