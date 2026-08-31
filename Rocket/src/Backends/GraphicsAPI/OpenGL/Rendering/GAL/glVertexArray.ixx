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

        void add_vbo(Ref<VertexBuffer> vbo, const GBufferLayout& layout) override;
        void set_ibo(Ref<IndexBuffer > ibo) override;
        void set_binding_divisor(uint32 binding, uint32 divisor) override;

        uint32 get() const override { return gal_id_; }
        const VertexBuffer& get_vbo(Size index) const override;
        const IndexBuffer& get_ibo() const override;

        void bind  () const override;
        void unbind() const override;
    private:
        uint32 gal_id_{};

        std::vector<Ref<VertexBuffer>> vbos_{}; // keep vbos alive
        Ref<IndexBuffer> ibo_{}; // keep the ibo alive(for draw-call)

        uint32 binding_index_{};
        uint32 attrib_index_{}; // next free attribute location
    };
}
