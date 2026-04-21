module;

#define GBUFFER_GENERAL_DECL \
    uint32 get_renderer_id() const override { return renderer_id_; } \
    void* map(Access access) override; \
    void unmap() override

export module Buffers:OpenGL;

import :Base;
import Types;

namespace rke
{
    class glVertexBuffer : public VertexBuffer
    {
    public:
        glVertexBuffer(const void* data, uint32 size);
        glVertexBuffer(uint32 size);
        ~glVertexBuffer() override;

        void set_data(const void* new_data, uint32 new_size) override;

        GBUFFER_GENERAL_DECL;
    private:
        uint32 renderer_id_{};
    };

    class glIndexBuffer : public IndexBuffer
    {
    public:
        glIndexBuffer(const void* data, uint32 count);
        ~glIndexBuffer();

        void set_data(const void* new_data, uint32 new_count) override;
        uint32 get_count() const override { return count_; }

        GBUFFER_GENERAL_DECL;
    private:
        uint32 renderer_id_{};
        uint32 count_;
    };

    class glUniformBuffer : public UniformBuffer
    {
    public:
        glUniformBuffer(uint32 size);
        ~glUniformBuffer() override;

        void set_data(const void* data, uint32 size, uint32 offset = 0) override;
        void bind(BindingPoint point) override;

        GBUFFER_GENERAL_DECL;
    private:
        uint32 renderer_id_{};
    };

    class glPixelBuffer : public PixelBuffer
    {
    public:
        glPixelBuffer(uint32 size);
        glPixelBuffer(const void* data, uint32 size);
        ~glPixelBuffer() override;

        void set_data(const void* data, uint32 size) override;
        void bind  (PixelBuffer::Usage usage) override;
        void unbind(PixelBuffer::Usage usage) override;

        GBUFFER_GENERAL_DECL;
    private:
        uint32 renderer_id_{};
    };
}
