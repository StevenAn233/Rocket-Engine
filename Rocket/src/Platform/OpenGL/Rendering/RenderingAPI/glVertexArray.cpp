module;

#include <glad/glad.h>

module VertexArray;
import :OpenGL;

import Buffers;
import Log;

import Instrumentor;

namespace {
    using namespace rke;

    static inline GLenum shader_data_type_to_GLenum(ShaderDataType type)
    {
        switch(type)
        {
        case ShaderDataType::Float:  return GL_FLOAT;
        case ShaderDataType::Float2: return GL_FLOAT;
        case ShaderDataType::Float3: return GL_FLOAT;
        case ShaderDataType::Float4: return GL_FLOAT;
        case ShaderDataType::Int:	 return GL_INT;
        case ShaderDataType::Int2:	 return GL_INT;
        case ShaderDataType::Int3:	 return GL_INT;
        case ShaderDataType::Int4:	 return GL_INT;
        case ShaderDataType::Mat3:	 return GL_FLOAT;
        case ShaderDataType::Mat4:	 return GL_FLOAT;
        case ShaderDataType::Bool:	 return GL_BOOL;
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
        glCreateVertexArrays(1, &renderer_id_);
        // A VAO can be for multiple VBOs
    }
    glVertexArray::~glVertexArray()
        { glDeleteVertexArrays(1, &renderer_id_); }

    void glVertexArray::add_vbo(const vbo_ref& vbo,
                                const BufferLayout& layout)
    {
        CORE_ASSERT(layout.get_elements().size(),
            u8"VertexBuffer: Layout has no elements!");

        glBindVertexArray(renderer_id_);
        glVertexArrayVertexBuffer
        // bind the vertex buffer to the vertex array
        (
            renderer_id_,           // VAO ID
            binding_index_,         // binding index
            vbo->get_renderer_id(), // VBO ID
            0,                      // offset(of the first element of the buffer)
            layout.get_stride()     // stride
        );

        const auto& elements = layout.get_elements();
        for(int i{}; i < elements.size(); i++)
        {
            uint32 attrib_index{ binding_index_ + i };
            // attrib_index: the corresponding index of location in shaders
            // e.g layout(location = 0) in vec3 v_position;
            // attribute here means a set of values which
            // represent one certain type of information e.g pos/normal/uv/color...

            switch (elements[i].type)
            {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::Mat3: // Mat is composed of floats
            case ShaderDataType::Mat4:
            {
                glVertexArrayAttribFormat (
                    renderer_id_,
                    attrib_index,
                    elements[i].count, // component count
                    shader_data_type_to_GLenum(elements[i].type),
                    elements[i].normalized ? GL_TRUE : GL_FALSE,
                    elements[i].offset);
                break;
            }
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::Bool:
            {
                glVertexArrayAttribIFormat (
                    renderer_id_,
                    attrib_index,
                    elements[i].count, // component count
                    shader_data_type_to_GLenum(elements[i].type),
                    elements[i].offset
                );
                break;
            }}
            glVertexArrayAttribBinding (
                renderer_id_, attrib_index, binding_index_
            ); // bind the attribute to the vbo
            glEnableVertexArrayAttrib (
                renderer_id_, attrib_index
            ); // and then enable it
        }
        binding_index_++;

        vbos_.push_back(vbo); // keep it alive
    }
    void glVertexArray::set_ibo(const ibo_ref& ibo)
    {
        glVertexArrayElementBuffer (
            renderer_id_,
            ibo->get_renderer_id()
        );
        ibo_ = ibo; // keep it alive
    }

    void glVertexArray::bind() const
    {
        glBindVertexArray(renderer_id_);
        glEnableVertexArrayAttrib(renderer_id_, 0);
        glEnableVertexArrayAttrib(renderer_id_, 1);
        //glVertexArrayAttribBinding(renderer_id_, 0, 0);
        //glVertexArrayAttribBinding(renderer_id_, 1, 0);
    }
    void glVertexArray::unbind() const { glBindVertexArray(0); }
}
