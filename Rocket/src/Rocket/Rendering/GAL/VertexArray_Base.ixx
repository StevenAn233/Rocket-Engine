module;

#include <vector>
#include "rke_macros.h"

export module VertexArray:Base;

import Types;
import HeapManager;
import GBuffers;

export namespace rke
{
    class RKE_API VertexArray
    {
    public:
        VertexArray() = default;
        virtual ~VertexArray() = default;

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&&) = delete;
        VertexArray& operator=(VertexArray&&) = delete;

        virtual void add_vbo(Ref<VertexBuffer> vbo, const GBufferLayout& layout) = 0;
        virtual void set_ibo(Ref<IndexBuffer > ibo) = 0;
        virtual void set_binding_divisor(uint32 binding, uint32 divisor) = 0;

        virtual uint32 get() const = 0;
        virtual const VertexBuffer& get_vbo(Size index) const = 0;
        virtual const IndexBuffer& get_ibo() const = 0;

        virtual void bind  () const = 0;
        virtual void unbind() const = 0;

        static Scope<VertexArray> create();
    };
}
