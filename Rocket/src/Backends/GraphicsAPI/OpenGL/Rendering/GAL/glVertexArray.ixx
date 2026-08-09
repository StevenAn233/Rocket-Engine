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

        void add_vbo(Ref<VertexBuffer> vbo, const BufferLayout& layout) override;
        void set_ibo(Ref<IndexBuffer > ibo) override;

        uint32 get() const override { return renderer_id_; }
        const VertexBuffer& get_vbo(Size index) const override;
        const IndexBuffer& get_ibo() const override;

        void bind  () const override;
        void unbind() const override;
    private:
        uint32 renderer_id_{};

        std::vector<Ref<VertexBuffer>> vbos_{}; // keep vbos alive
        Ref<IndexBuffer> ibo_{}; // keep the ibo alive(for draw-call)

        uint32 binding_index_{};
    };
}
