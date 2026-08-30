module;

#include <vector>

export module FrameBuffer:OpenGL;

import :Base;
import Types;
import GBuffers;

namespace rke
{
    class glFrameBuffer : public FrameBuffer
    {
    public:
        glFrameBuffer(Specification spec);
        ~glFrameBuffer() override;

        void clear() override;

        void resize(uint32 w, uint32 h) override;
        void set_samples(uint32 samples) override;
        int  read_pixel(uint32 color_attach_index, int x, int y) override;
        void clear_pbo() override;

        uint32 get_gal_id() const override { return gal_id_; }
        const GTexture2D* get_gtexture_attached(int index = 0) const override;
        const Specification& get_specification() const override { return spec_; }
    private:
        bool zero_sized() const override;
        bool over_sized() const override;

        void bind() override;
        void unbind() override;

        void invalidate();
    private:
        Specification spec_;

        uint32 gal_id_{};
        std::vector<uint32> attachments_{};

        uint32 msaa_renderer_id_{};
        std::vector<uint32> msaa_attachments_{};

        std::vector<Scope<GTexture2D>> output_textures_{}; // serve as view/wrapper here

    // for read_pixel()
        Ref<PixelBuffer> pixel_pbo_{};
        int last_read_pixel_value_{ -1 };
        bool pbo_has_pending_request_{ false };
    };
}
