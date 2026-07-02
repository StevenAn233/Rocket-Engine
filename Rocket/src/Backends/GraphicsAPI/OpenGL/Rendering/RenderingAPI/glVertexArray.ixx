export module VertexArray:OpenGL;

import :Base;
import Types;

namespace rke
{
    class glVertexArray : public VertexArray
    {
    public:
        glVertexArray();
        ~glVertexArray() override;

        void add_vbo(const Ref<VertexBuffer>& vbo, const BufferLayout& layout) override;
        void set_ibo(const Ref<IndexBuffer >& ibo) override;

        uint32 get() const override { return renderer_id_; }
        const std::vector<Ref<VertexBuffer>>& get_vbos() const override { return vbos_; }
        const Ref<IndexBuffer>& get_ibo() const override { return ibo_; }

        void bind  () const override;
        void unbind() const override;
    private:
        uint32 renderer_id_{};

        std::vector<Ref<VertexBuffer>> vbos_{}; // keep vbos alive
        Ref<IndexBuffer> ibo_{}; // keep the ibo alive(for draw-call)

        uint32 binding_index_{};
    };
}
