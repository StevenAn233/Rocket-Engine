module;

#include <stb_image.h>
#include <stb_truetype.h>

module Renderer2D;

import Log;
import Path;
import RenderCommand;
import BindingPoint;
import AssetsManager;
import MathUtils;
import FileUtils;
import Application;
import Instrumentor;
import Window;

namespace {
    using namespace rke;
    static_assert(Renderer2D::max_texture_slots >= 2,
        u8"Renderer2D: Texture slots must be more than 2!");
    static_assert(Renderer2D::max_quads >= 0,
        u8"Renderer2D: Max quads must be more than 0!");
}

namespace rke
{
    Scope<Shader> Renderer2D::s_shader{};

    void Renderer2D::init()
    {
        Path shader_path{ file::assets_dir() / u8"shaders" / u8"renderer2D.rkshdr" };
        s_shader = Shader::create(shader_path);
    }

    Renderer2D::Renderer2D(Window* context) : context_(context)
    {
        CORE_ASSERT(context_, u8"Renderer2D: Context window null!");

        default_texture_ = Texture2D::create(1, 1, Texture::Format::RGBA8);
        uint32 white_pixel{ 0xFFFFFFFF }; // 4 bytes
        default_texture_->set_data(&white_pixel, sizeof(white_pixel));
        texture_slots_[0] = default_texture_.get();

        context_data_ = create_scope<ContextData>();
        context_data_->vao = VertexArray ::create();
        context_data_->vbo = VertexBuffer::create(max_vertices * sizeof(QuadVertexProps));
        // huge, empty vbo(only with size)
        context_data_->ubo = UniformBuffer::create(sizeof(CameraData));

        rke::BufferLayout quad_vertex_layout
        {
            { u8"a_position", rke::ShaderDataType::Float3 },
            { u8"a_color"   , rke::ShaderDataType::Float4 },
            { u8"a_uv_coord", rke::ShaderDataType::Float2 },
            { u8"a_tiling_factor", rke::ShaderDataType::Float },

            { u8"a_tex_id"	   , rke::ShaderDataType::Int },
            { u8"a_if_tex_grey", rke::ShaderDataType::Int },
            { u8"a_is_font"	   , rke::ShaderDataType::Int },
            { u8"a_entity_id"  , rke::ShaderDataType::Int }
        };
        context_data_->vao->add_vbo(context_data_->vbo, quad_vertex_layout);

        auto* indices{ new uint32[max_indices] }; // only malloc during init
        uint32 offset{};
        for(uint32 i{}; i < max_indices; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
            // 4 for 4 vertices of a quad
            // 6 for 6 vertices of two triangles
        }
        context_data_->ibo = IndexBuffer::create(indices, max_indices);
        delete[] indices; // per-context
        context_data_->vao->set_ibo(context_data_->ibo);
    }

    Renderer2D::~Renderer2D() {}

    void Renderer2D::begin_camera(const glm::mat4& view_projection)
    {
        RKE_PROFILE_FUNCTION();

        auto& data{ *context_data_ };
        data.ubo->bind(BindingPoint::UBO_Camera);
        data.camera_data.view_proj = view_projection;
        data.ubo->set_data(&(data.camera_data), sizeof(CameraData));
    #ifdef RKE_ENABLE_STATISTICS
        stats_.cam_set_count++;
    #endif
    }

    void Renderer2D::begin_scene()
    {
        in_scene_ = true;
        s_shader->bind();
        start_batch();
    }

    void Renderer2D::end_scene()
    {
        flush();
        s_shader->unbind();
        in_scene_ = false;
    }

    void Renderer2D::draw_quad(const QuadProps& props)
    {
        RKE_PROFILE_FUNCTION();
        CORE_ASSERT(in_scene_, u8"Renderer2D: "
            u8"Call of draw_quad should be between begin_sence and end_sence!");

        auto& data{ *context_data_ };
        if(data.index_count >= max_indices || texture_slot_index_ >= max_texture_slots)
            { flush(); start_batch(); }

        // calculate position(CPU side)
        glm::mat4 transform {
            glm::translate(glm::mat4(1.0f), props.position) *
            glm::mat4_cast(glm::quat(glm::radians(props.rotation))) *
            glm::scale(glm::mat4(1.0f), { props.size.x, props.size.y, 0.0f })
        };

        // find texture id
        int tex_index{ 0 }; // white texture(default)
        if(props.texture)
        {
            bool found{ false };
            for(uint32 i{}; i < texture_slot_index_; i++)
            {
                if(texture_slots_[i] == props.texture)
                {
                    tex_index = i;
                    found = true; break;
                }
            }

            if(!found) {
                tex_index = texture_slot_index_;
                texture_slots_[texture_slot_index_] = props.texture;
                texture_slot_index_++;
            }
        }

        for(int i{}; i < 4; i++)
        {
            quad_vertex_ptr_->position = glm::vec3(transform * quad_vertex_pos[i]);
            quad_vertex_ptr_->color	   = math::srgb_to_linear(props.color);
            quad_vertex_ptr_->uv_coord = props.uv_coords[i];
            quad_vertex_ptr_->tiling_factor = props.tiling_factor;
            quad_vertex_ptr_->tex_id      = tex_index;
            quad_vertex_ptr_->if_tex_grey = static_cast<int>(props.make_tex_gray);
            quad_vertex_ptr_->is_font	  = static_cast<int>(props.is_font);
            quad_vertex_ptr_->entity_id	  = props.entity_id;
            quad_vertex_ptr_++; // stride: QuadVertexProps
        }
        data.index_count += 6;
    #ifdef RKE_ENABLE_STATISTICS
        stats_.quad_count++;
    #endif
    }

    void Renderer2D::draw_text(const String& text,
        Font& font, glm::vec3 pos, float scale, glm::vec4 color)
    {
        RKE_PROFILE_FUNCTION();

        auto* char_data{ static_cast<const stbtt_packedchar*>(font.get_char_data()) };
        float x{}, y{};
        for(const char* c{ text.raw() }; *c; c++) // should be safe with String
        {
            stbtt_aligned_quad quad{};
            stbtt_GetPackedQuad(char_data,
                font.get_atlas_size(), font.get_atlas_size(),
                static_cast<int>(*c - 32), &x, &y, &quad, 0);

            if(*c == ' ') continue;

            QuadProps props{};
            props.position = {
                ( (quad.x0 + quad.x1) * (scale / font.get_font_size()) / 2.0f) + pos.x,
                (-(quad.y0 + quad.y1) * (scale / font.get_font_size()) / 2.0f) + pos.y, // flip manully
                pos.z
            };
            props.size = {
                -(quad.x1 - quad.x0) * (scale / font.get_font_size()), // flip manully
                 (quad.y1 - quad.y0) * (scale / font.get_font_size())
            };
            props.color = color;
            props.texture = font.get_font_atlas();

            props.uv_coords[0] = { quad.s1, quad.t1 };
            props.uv_coords[1] = { quad.s0, quad.t1 };
            props.uv_coords[2] = { quad.s0, quad.t0 };
            props.uv_coords[3] = { quad.s1, quad.t0 };

            props.is_font = true;

            draw_quad(props);
        }
    }

    void Renderer2D::start_batch()
    {
        RKE_PROFILE_FUNCTION();

        // refresh the huge data buffer
        auto& data{ *context_data_ };
        data.index_count = 0;
        texture_slot_index_ = 1; // set to the head

        if(quad_vertex_ptr_ != nullptr) 
        {
            data.vbo->unmap();
            quad_vertex_ptr_ = nullptr;
            CORE_ERROR(u8"Renderer2D: VBO was explicitly "
                u8"unmapped before re-mapping. Check flush logic!");
        }
        quad_vertex_ptr_ = reinterpret_cast<QuadVertexProps*>
            (data.vbo->map(GBuffer::Access::Write));
        CORE_ASSERT(quad_vertex_ptr_, u8"Renderer2D: Failed to map vertex buffer!");
    }

    void Renderer2D::flush()
    {
        RKE_PROFILE_FUNCTION();

        auto& data{ *context_data_ };

        if(quad_vertex_ptr_)
        {
            data.vbo->unmap();
            quad_vertex_ptr_ = nullptr;
        }
        if(data.index_count == 0) return;

        // bind textures
        for(uint32 i{}; i < texture_slot_index_; i++)
            texture_slots_[i]->bind(static_cast<uint32>(BindingPoint::Sampler2D_0) + i);

        data.vao->bind();
        app().render_command().draw_indexed(data.index_count);
        data.vao->unbind();
    #ifdef RKE_ENABLE_STATISTICS
        stats_.drawcall_count++;
    #endif
    }
}
