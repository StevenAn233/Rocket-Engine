module;

#include <glad/glad.h>

export module Texture:OpenGL;

import :Base;
import Types;

namespace rke
{
    class glTexture2D : public Texture2D
    {
    public:
        glTexture2D(uint32 w, uint32 h, Format format, FiltFormat filt);
        glTexture2D(const Path& filepath, FiltFormat filt, WrapFormat wrap, bool srgb);
        glTexture2D(uint32 renderer_id, uint32 w, uint32 h, Format format);
        ~glTexture2D() override;

        uint32 get_width () const override { return width_;  }
        uint32 get_height() const override { return height_; }

        uint32 get_channels() const override { return channels_; }
        uint32 get_renderer_id() const override { return renderer_id_; }

        void set_data(void* data, uint32 size) override;

        void bind(BindingPoint point) const override;
        void bind(uint32 slot) const override;
    private:
        bool owns_texture_; // may need to change

        uint32 width_{}, height_{};
        uint32 renderer_id_{};

        GLenum internal_format_{};
        GLenum data_format_{};
        GLenum pixel_type_ {};
        uint32 channels_{};
    };
}
