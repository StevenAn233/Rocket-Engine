module;

#include <vector>
#include <variant>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module FrameBuffer:Base;

import Types;
import Texture;
import HeapManager;

export namespace rke
{
    class RKE_API FrameBuffer
    {
    public:
        struct RKE_API TextureSpecification
        {
            using ClearValue = std::variant<std::monostate, glm::vec4, glm::vec3, int, float>;
            enum class LoadOp { CLEAR, LOAD };

            TextureSpecification()
                : format(Texture::Format::None), clear_value({})
                , load_op(LoadOp::CLEAR), filtering(Texture::FiltFormat::Linear) {};
            TextureSpecification(Texture::Format tex_format,
                                 ClearValue clear_value = {},
                                 LoadOp op = LoadOp::CLEAR,
                                 Texture::FiltFormat filt = Texture::FiltFormat::Linear)
                : format(tex_format), clear_value(std::move(clear_value))
                , load_op(op), filtering(filt) {}

            Texture::Format format;
            ClearValue clear_value;
            LoadOp load_op;

            Texture::FiltFormat filtering;
        //  Texture::WrapFormat wrapping;
        };

        struct RKE_API AttachmentSpecification
        {
            AttachmentSpecification() : texture_specs() {};
            AttachmentSpecification(std::initializer_list<TextureSpecification> specs)
                : texture_specs(specs) {}

            std::vector<TextureSpecification> texture_specs;
        };

        struct RKE_API Specification
        {
            uint32 width{}, height{};
            uint32 samples{ 1 };

            bool swap_chain_target{ false };
            // switch this to 'true' if you want to render directly on the screen

            AttachmentSpecification attachment_spec{};
        };

        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer&) = delete;
        FrameBuffer(FrameBuffer&&) = delete;
        FrameBuffer& operator=(FrameBuffer&&) = delete;

        template<typename Func>
        requires std::invocable<Func>
        void clear_to_upload(Func&& func)
        {
            if(zero_sized() || over_sized()) { clear(); return; }
            bind();
            std::invoke(std::forward<Func>(func));
            unbind();
        }

        virtual void clear() = 0;

        virtual void resize(uint32 w, uint32 h) = 0;
        virtual void set_samples(uint32 samples) = 0;
        virtual int  read_pixel(uint32 attach_index, int x, int y) = 0;
        virtual void clear_pbo() = 0;

        virtual uint32 get_renderer_id() const = 0;
        virtual uint32 get_attachment(int index = 0) const = 0;
        virtual const Texture2D* get_texture(int index = 0) const = 0;
        virtual const Specification& get_specification() const = 0;

        static Ref<FrameBuffer> create(Specification spec);
    protected:
        FrameBuffer() = default;
        virtual ~FrameBuffer() = default;
    private:
        virtual bool zero_sized() const = 0;
        virtual bool over_sized() const = 0;
        virtual void bind  () = 0;
        virtual void unbind() = 0;
    };
}
