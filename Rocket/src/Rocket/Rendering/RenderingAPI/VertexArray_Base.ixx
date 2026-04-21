module;

#include <vector>
#include "rke_macros.h"

export module VertexArray:Base;

import Types;
import HeapManager;
import Buffers;

namespace rke
{
    using vbo_ref_arr = std::vector<Ref<VertexBuffer>>;

    using vbo_ref = Ref<VertexBuffer>;
    using ibo_ref = Ref<IndexBuffer >;
}

export namespace rke
{
    class RKE_API VertexArray
    {
    public:
        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&&) = delete;
        VertexArray& operator=(VertexArray&&) = delete;

        virtual void add_vbo(const vbo_ref& vbo, const BufferLayout& layout) = 0;
        virtual void set_ibo(const ibo_ref& ibo) = 0;

        virtual uint32 get() const = 0;
        virtual const vbo_ref_arr& get_vbos() const = 0;
        virtual const ibo_ref&	   get_ibo () const = 0;

        virtual void bind  () const = 0;
        virtual void unbind() const = 0;

        static Ref<VertexArray> create();
    protected:
        VertexArray() = default;
        virtual ~VertexArray() = default;
    };
}
