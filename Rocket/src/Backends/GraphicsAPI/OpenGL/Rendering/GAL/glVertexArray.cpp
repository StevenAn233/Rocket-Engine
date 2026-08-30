module;

#include <glad/glad.h>

module VertexArray;
import :OpenGL;

import Log;
import GBuffers;
import Instrumentor;

namespace {
    using namespace rke;

    static inline GLenum shader_data_type_to_GLenum(GShaderDataType type)
    {
        switch(type)
        {
        case GShaderDataType::Float:  return GL_FLOAT;
        case GShaderDataType::Float2: return GL_FLOAT;
        case GShaderDataType::Float3: return GL_FLOAT;
        case GShaderDataType::Float4: return GL_FLOAT;
        case GShaderDataType::Int:	 return GL_INT;
        case GShaderDataType::Int2:	 return GL_INT;
        case GShaderDataType::Int3:	 return GL_INT;
        case GShaderDataType::Int4:	 return GL_INT;
        case GShaderDataType::Mat3:	 return GL_FLOAT;
        case GShaderDataType::Mat4:	 return GL_FLOAT;
        case GShaderDataType::Bool:	 return GL_BOOL;
        default:
            CORE_ASSERT(false, u8"glVertexArray: Unknown shader data type!");
            std::unreachable();
        }
    }
}

namespace rke
{
    glVertexArray::glVertexArray()
    {
        glCreateVertexArrays(1, &gal_id_);
        // A VAO can be for multiple VBOs
    }
    glVertexArray::~glVertexArray()
        { glDeleteVertexArrays(1, &gal_id_); }

    void glVertexArray::add_vbo(Ref<VertexBuffer> vbo, const GBufferLayout& layout)
    {
        CORE_ASSERT(layout.get_elements().size(),
            u8"VertexBuffer: Layout has no elements!");

        glBindVertexArray(gal_id_);
        glVertexArrayVertexBuffer ( // bind the vertex buffer to the vertex array
            gal_id_,           // VAO ID
            binding_index_,         // binding index
            vbo->get_gal_id(), // VBO ID
            0,                      // offset(of the first element of the buffer)
            layout.get_stride()     // stride
        );

        const auto& elements{ layout.get_elements() };
        for(int i{}; i < elements.size(); i++)
        {
            uint32 attrib_index{ binding_index_ + i };
            // attrib_index: the corresponding index of location in shaders
            // e.g layout(location = 0) in vec3 v_position;
            // attribute here means a set of values which
            // represent one certain type of information e.g pos/normal/uv/color...

            switch(elements[i].type)
            {
            case GShaderDataType::Float:
            case GShaderDataType::Float2:
            case GShaderDataType::Float3:
            case GShaderDataType::Float4:
            case GShaderDataType::Mat3: // Mat is composed of floats
            case GShaderDataType::Mat4:
                glVertexArrayAttribFormat (
                    gal_id_,
                    attrib_index,
                    elements[i].count, // component count
                    shader_data_type_to_GLenum(elements[i].type),
                    elements[i].normalized ? GL_TRUE : GL_FALSE,
                    elements[i].offset);
                break;
            case GShaderDataType::Int:
            case GShaderDataType::Int2:
            case GShaderDataType::Int3:
            case GShaderDataType::Int4:
            case GShaderDataType::Bool:
                glVertexArrayAttribIFormat (
                    gal_id_,
                    attrib_index,
                    elements[i].count, // component count
                    shader_data_type_to_GLenum(elements[i].type),
                    elements[i].offset);
                break;
            }
            glVertexArrayAttribBinding(gal_id_, attrib_index, binding_index_);
            glEnableVertexArrayAttrib (gal_id_, attrib_index);
        }
        binding_index_++;

        vbos_.push_back(std::move(vbo)); // keep it alive
    }

    void glVertexArray::set_ibo(Ref<IndexBuffer> ibo)
    {
        glVertexArrayElementBuffer(gal_id_, ibo->get_gal_id());
        ibo_ = std::move(ibo); // keep it alive
    }

    const VertexBuffer& glVertexArray::get_vbo(Size index) const
    {
        CORE_ASSERT(index < vbos_.size(), u8"glVertexArray: Out of bound!");
        const VertexBuffer* ptr{ vbos_[index].get() };
        CORE_ASSERT(ptr, u8"glVertexArray: Object null!");
        return *ptr;
    }

    const IndexBuffer& glVertexArray::get_ibo() const
    {
        const IndexBuffer* ptr{ ibo_.get() };
        CORE_ASSERT(ptr, u8"glVertexArray: Object null!");
        return *ptr;
    }

    void glVertexArray::bind() const
    {
        glBindVertexArray(gal_id_);
        glEnableVertexArrayAttrib(gal_id_, 0);
        glEnableVertexArrayAttrib(gal_id_, 1);
        // glVertexArrayAttribBinding(gal_id_, 0, 0);
        // glVertexArrayAttribBinding(gal_id_, 1, 0);
    }

    void glVertexArray::unbind() const { glBindVertexArray(0); }
}
