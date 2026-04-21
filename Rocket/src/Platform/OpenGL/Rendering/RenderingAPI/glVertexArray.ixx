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

        void add_vbo(const vbo_ref& vbo, const BufferLayout& layout) override;
        void set_ibo(const ibo_ref& ibo) override;

        uint32 get() const override { return renderer_id_; }
        const vbo_ref_arr& get_vbos() const override { return vbos_; }
        const ibo_ref& get_ibo() const override { return ibo_; }

        void bind() const override;
        void unbind() const override;
    private:
        uint32 renderer_id_{};

        vbo_ref_arr vbos_{}; // keep vbos alive
        ibo_ref	ibo_{}; // keep the ibo alive(and for darw calling)

        uint32 binding_index_{};
    };
}
