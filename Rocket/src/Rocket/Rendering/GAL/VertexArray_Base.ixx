module;

#include <vector>
#include "rke_macros.h"

export module VertexArray:Base;

import Types;
import HeapManager;
import Buffers;

export namespace rke
{
    class RKE_API VertexArray
    {
    public:
        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&&) = delete;
        VertexArray& operator=(VertexArray&&) = delete;

        virtual void add_vbo(const Ref<VertexBuffer>& vbo, const BufferLayout& layout) = 0;
        virtual void set_ibo(const Ref<IndexBuffer >& ibo) = 0;

        virtual uint32 get() const = 0;
        virtual const std::vector<Ref<VertexBuffer>>& get_vbos() const = 0;
        virtual const Ref<IndexBuffer>& get_ibo() const = 0;

        virtual void bind  () const = 0;
        virtual void unbind() const = 0;

        static Ref<VertexArray> create();
    protected:
        VertexArray() = default;
        virtual ~VertexArray() = default;
    };
}
