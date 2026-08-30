module;

#include <glad/glad.h>

export module GTexture:OpenGL;

import :Base;
import Types;

namespace rke
{
    class glGTexture2D : public GTexture2D
    {
    public:
        glGTexture2D(uint32 w, uint32 h, Format format,
            const void* data = nullptr,
            FiltFormat filt = FiltFormat::Linear,
            WrapFormat wrap = WrapFormat::Clamp2Edge);
        glGTexture2D(uint32 gal_id, uint32 w, uint32 h, Format format);
        ~glGTexture2D() override;

        uint32 get_width () const override { return width_;  }
        uint32 get_height() const override { return height_; }

        uint32 get_channels() const override { return channels_; }
        uint32 get_gal_id() const override { return gal_id_; }

        void set_data(void* data, uint32 size) override;

        void bind(BindingPoint point) const override;
        void bind(uint32 slot) const override;
    private:
        bool owns_texture_; // TO MODIFY

        uint32 width_{}, height_{};
        uint32 gal_id_{};

        GLenum internal_format_{};
        GLenum data_format_{};
        GLenum pixel_type_ {};
        uint32 channels_{};
    };
}
