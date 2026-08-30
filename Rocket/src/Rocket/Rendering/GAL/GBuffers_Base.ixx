module;

#include <vector>
#include <initializer_list>
#include "rke_macros.h"

export module GBuffers:Base;

import String;
import Types;
import HeapManager;
import BindingPoint;

export namespace rke
{
    enum class GShaderDataType
    {
        None = 0, Float, Float2, Float3, Float4,
        Int, Int2, Int3, Int4, Mat3, Mat4, Bool
    };
}

namespace {
    using namespace rke;

    static inline uint32 sizeoftype(GShaderDataType type)
    {
        switch(type)
        {
        case GShaderDataType::Float:  return 4;
        case GShaderDataType::Float2: return 8;
        case GShaderDataType::Float3: return 12;
        case GShaderDataType::Float4: return 16;
        case GShaderDataType::Int:	 return 4;
        case GShaderDataType::Int2:	 return 8;
        case GShaderDataType::Int3:	 return 12;
        case GShaderDataType::Int4:	 return 16;
        case GShaderDataType::Mat3:	 return 36;
        case GShaderDataType::Mat4:	 return 64;
        case GShaderDataType::Bool:	 return 1;
        }
        return 0;
    }

    static inline uint32 countoftype(GShaderDataType type)
    {
        switch(type)
        {
        case GShaderDataType::Float:  return 1;
        case GShaderDataType::Float2: return 2;
        case GShaderDataType::Float3: return 3;
        case GShaderDataType::Float4: return 4;
        case GShaderDataType::Int:	 return 1;
        case GShaderDataType::Int2:	 return 2;
        case GShaderDataType::Int3:	 return 3;
        case GShaderDataType::Int4:	 return 4;
        case GShaderDataType::Mat3:	 return 9;
        case GShaderDataType::Mat4:	 return 16;
        case GShaderDataType::Bool:	 return 1;
        }
        return 0;
    }
}

export namespace rke
{
// Buffer Layout
    struct RKE_API GBufferElements
    {
        String name;
        GShaderDataType type;

        uint32 count;
        bool normalized;
        uint32 offset;

        GBufferElements(const String&  _name,
                       GShaderDataType _type,
                       bool _normalized = false)
            : name(_name), type(_type)
            , count(countoftype(_type))
            , normalized( _normalized), offset(0) {}
        // offset will be set in GBufferLayout::
        // set_elements_offset_and_my_stride();
        GBufferElements(const GBufferElements&) = default;
    };

    class RKE_API GBufferLayout
    {
    public:
        GBufferLayout(std::initializer_list<GBufferElements> elements)
            : elements_(elements) { set_elements_offset_and_my_stride(); }

        const std::vector<GBufferElements>& get_elements() const { return elements_; }
        uint32 get_stride() const { return stride_; } // for constexpr obj
    private:
        void set_elements_offset_and_my_stride()
        {
            uint32 offset{};
            for(auto& element : elements_)
            {
                element.offset = offset;
                offset  += sizeoftype(element.type);
                stride_ += sizeoftype(element.type);
            }
        }
    private:
        std::vector<GBufferElements> elements_;
        uint32 stride_{};
    };

// GBuffers
    class RKE_API GBuffer
    {
    public:
        enum class Access { Read, Write, ReadWrite };

        GBuffer(const GBuffer&) = delete;
        GBuffer& operator=(const GBuffer&) = delete;
        GBuffer(GBuffer&&) = delete;
        GBuffer& operator=(GBuffer&&) = delete;

        virtual uint32 get_gal_id() const = 0;

        virtual void* map(Access access) = 0;
        virtual void unmap() = 0;
    protected:
        GBuffer() = default;
        virtual ~GBuffer() = default;
    };

    class RKE_API VertexBuffer : public GBuffer
    {
    public:
        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&&) = delete;
        VertexBuffer& operator=(VertexBuffer&&) = delete;

        virtual void set_data(const void* new_data, uint32 new_size) = 0;

        static Ref<VertexBuffer> create(const void* data, uint32 size);
        static Ref<VertexBuffer> create(uint32 size);
    protected:
        VertexBuffer() = default;
        virtual ~VertexBuffer() = default;
    };

    class RKE_API IndexBuffer : public GBuffer
    {
    public:
        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;
        IndexBuffer(IndexBuffer&&) = delete;
        IndexBuffer& operator=(IndexBuffer&&) = delete;

        virtual void set_data(const void* new_data, uint32 new_count) = 0;
        virtual uint32 get_count() const = 0;

        static Ref<IndexBuffer> create(const void* data, uint32 count);
    protected:
        IndexBuffer() = default;
        virtual ~IndexBuffer() = default;
    };

    class RKE_API UniformBuffer : public GBuffer
    {
    public:
        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;
        UniformBuffer(UniformBuffer&&) = delete;
        UniformBuffer& operator=(UniformBuffer&&) = delete;

        virtual void set_data(const void* data, uint32 size, uint32 offset = 0) = 0;
        virtual void bind(BindingPoint point) = 0;

        static Ref<UniformBuffer> create(uint32 size);
    protected:
        UniformBuffer() = default;
        virtual ~UniformBuffer() = default;
    };

    class RKE_API PixelBuffer : public GBuffer
    {
    public:
        enum class Usage { Pack, Unpack };

        PixelBuffer(const PixelBuffer&) = delete;
        PixelBuffer& operator=(const PixelBuffer&) = delete;
        PixelBuffer(PixelBuffer&&) = delete;
        PixelBuffer& operator=(PixelBuffer&&) = delete;

        virtual void set_data(const void* data, uint32 size) = 0;
        virtual void bind  (Usage usage) = 0;
        virtual void unbind(Usage usage) = 0;

        static Ref<PixelBuffer> create(uint32 size);
        static Ref<PixelBuffer> create(const void* data, uint32 size);
    protected:
        PixelBuffer() = default;
        virtual ~PixelBuffer() = default;
    };
}
