module;

#include <stb_image.h>
#include <stb_truetype.h>

module Renderer2D;

import Log;
import Path;
import VertexArray;
import Shader;
import Buffers;
import Application;
import RenderCommand;
import BindingPoint;
import AssetsManager;
import MathUtils;

import Instrumentor;

// maybe need to remove EDITOR ONLY stuffs in the future
namespace {
    using namespace rke;

    struct QuadVertexProps
    {
        glm::vec3 position { 0.0f };
        glm::vec4 color	   { 0.0f };
        glm::vec2 uv_coord { 0.0f };
        float tiling_factor{ 1.0f };

        int tex_id	   { 0 };
        int if_tex_grey{ 0 };
        int is_font	   { 0 };
        int entity_id  {-1 }; // EDITOR ONLY
    };

    struct CameraData { glm::mat4 view_proj{ 1.0f }; };

    struct PerContextData
    {
        Ref<VertexArray  > vao{};
        Ref<VertexBuffer > vbo{}; // a huge vbo(for one context)
        Ref<IndexBuffer  > ibo{}; // a huge ibo(for one context)
        Ref<UniformBuffer> ubo{};

        CameraData camera_buffer{};
        uint32 index_count{};
    };

    struct Renderer2DStorage
    {
        static constexpr uint32 MAX_QUADS	{ 10000 };
        static constexpr uint32 MAX_VERTICES{ MAX_QUADS * 4 };
        static constexpr uint32 MAX_INDICES { MAX_QUADS * 6 };
        static constexpr uint32 MAX_TEXTURE_SLOTS{ 32 };

        std::array<glm::vec4, 4> quad_vertex_pos
        {
            glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
            glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f)
        };

        Ref<Shader> shader{};
        Ref<Texture2D> default_texture{};

        std::array<Texture2D*, MAX_TEXTURE_SLOTS> texture_slots{};
        uint32 texture_slot_index{ 1 }; // 0 for default texture

        std::unordered_map<uintptr, PerContextData> context_data{};
        QuadVertexProps* quad_vertex_ptr{ nullptr };

        Renderer2D::Statistics stats{};
    };

    static Renderer2DStorage s_data{};
    static bool s_in_scene{ false };
}

namespace rke
{
    

// public
    void Renderer2D::init()
    {
        Path shader_path{ Application::get().asset_path
            (Path(u8"shaders") / u8"renderer2D.rkshdr") };
        s_data.shader = Shader::create(shader_path);

        CORE_ASSERT(s_data.MAX_TEXTURE_SLOTS >= 2,
            u8"Renderer2D: Texture slots must be more than 2!");
        CORE_ASSERT(s_data.MAX_QUADS >= 0,
            u8"Renderer2D: Max quads must be more than 0!");

        s_data.default_texture = Texture2D::create(1, 1, Texture::Format::RGBA8);
        uint32 white_pixel{ 0xFFFFFFFF }; // 4 bytes
        s_data.default_texture->set_data(&white_pixel, sizeof(white_pixel));
        s_data.texture_slots[0] = s_data.default_texture.get();
    }

    void Renderer2D::shutdown() {}

    void Renderer2D::register_context()
    {
        auto handle{ Application::get()
            .get_window_lib()->get_current_context().get_integral() };
        CORE_ASSERT(handle, u8"Renderer2D: Cannot register a null context!");
        if(s_data.context_data.count(handle)) return;

        PerContextData data{};
        data.vao = VertexArray ::create();
        data.vbo = VertexBuffer::create(s_data.MAX_VERTICES * sizeof(QuadVertexProps));
         // huge, empty vbo(only with size)
        data.ubo = UniformBuffer::create(sizeof(CameraData));

        static const rke::BufferLayout quad_vertex_layout
        {
            { u8"a_position", rke::ShaderDataType::Float3 },
            { u8"a_color"   , rke::ShaderDataType::Float4 },
            { u8"a_uv_coord", rke::ShaderDataType::Float2 },
            { u8"a_tiling_factor", rke::ShaderDataType::Float },

            { u8"a_tex_id"	   , rke::ShaderDataType::Int },
            { u8"a_if_tex_grey", rke::ShaderDataType::Int },
            { u8"a_is_font"	   , rke::ShaderDataType::Int },
            { u8"a_entity_id"  , rke::ShaderDataType::Int } // EDITOR ONLY
        };
        data.vao->add_vbo(data.vbo, quad_vertex_layout);

        auto* indices{ new uint32[s_data.MAX_INDICES] }; // only malloc during init
        uint32 offset{};
        for(uint32 i{}; i < s_data.MAX_INDICES; i += 6)
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
        data.ibo = IndexBuffer::create(indices, s_data.MAX_INDICES);
        delete[] indices; // per-context
        data.vao->set_ibo(data.ibo);

        s_data.context_data[handle] = data;
        CORE_INFO(u8"Renderer2D: Registered new context and created vao.");
    }

    void Renderer2D::begin_camera(const glm::mat4& view_projection)
    {
        RKE_PROFILE_FUNCTION();

        auto handle{ Application::get()
            .get_window_lib()->get_current_context().get_integral() };
        CORE_ASSERT(s_data.context_data.count(handle), u8"Renderer2D: Unregistered context!");
        auto& data{ s_data.context_data.at(handle) };

        data.ubo->bind(BindingPoint::UBO_Camera);
        data.camera_buffer.view_proj = view_projection;
        data.ubo->set_data(&data.camera_buffer, sizeof(CameraData));
    #ifdef RKE_ENABLE_STATISTICS
        s_data.stats.cam_set_count++;
    #endif
    }

    void Renderer2D::begin_scene()
    {
        RKE_PROFILE_FUNCTION();

        s_in_scene = true;
        s_data.shader->bind();
        start_batch();
    }

    void Renderer2D::end_scene()
    {
        RKE_PROFILE_FUNCTION();
        s_in_scene = false;

        flush();
    }

    void Renderer2D::draw_quad(const Renderer2D::QuadProps& props)
    {
        RKE_PROFILE_FUNCTION();
        CORE_ASSERT(s_in_scene, u8"Renderer2D: Call of draw_quad"
            u8" should be between begin_sence and end_sence!");

        auto handle{ Application::get()
            .get_window_lib()->get_current_context().get_integral() };
        CORE_ASSERT(s_data.context_data.count(handle),
            u8"Renderer2D: Drawing on an unregistered context!");
        auto& data{ s_data.context_data.at(handle) };

        if(data.index_count	>= s_data.MAX_INDICES ||
           s_data.texture_slot_index >= s_data.MAX_TEXTURE_SLOTS)
            { flush(); start_batch(); }

        // calculate position(CPU side)
        glm::mat4 transform {
            glm::translate(glm::mat4(1.0f), props.position)
          * glm::mat4_cast(glm::quat(glm::radians(props.rotation)))
          * glm::scale	  (glm::mat4(1.0f), { props.size.x, props.size.y, 0.0f })
        };

        // find texture id
        int tex_index{ 0 }; // white texture(default)
        if(props.texture)
        {
            bool found{ false };
            for(uint32 i{}; i < s_data.texture_slot_index; i++)
            {
                if(s_data.texture_slots[i] == props.texture)
                {
                    tex_index = i;
                    found = true; break;
                }
            }

            if(!found) {
                tex_index = s_data.texture_slot_index;
                s_data.texture_slots[s_data.texture_slot_index] = props.texture;
                s_data.texture_slot_index++;
            }
        }

        for(int i{}; i < 4; i++)
        {
            s_data.quad_vertex_ptr->position	  = glm::vec3(transform * s_data.quad_vertex_pos[i]);
            s_data.quad_vertex_ptr->color		  = math::srgb_to_linear(props.color);
            s_data.quad_vertex_ptr->uv_coord	  = props.uv_coords[i];
            s_data.quad_vertex_ptr->tiling_factor = props.tiling_factor;
            s_data.quad_vertex_ptr->tex_id		  = tex_index;
            s_data.quad_vertex_ptr->if_tex_grey	  = static_cast<int>(props.make_tex_gray);
            s_data.quad_vertex_ptr->is_font		  = static_cast<int>(props.is_font);
            s_data.quad_vertex_ptr->entity_id	  = props.entity_id; // EDITOR ONLY
            s_data.quad_vertex_ptr++; // stride: QuadVertexProps
        }
        data.index_count += 6;
    #ifdef RKE_ENABLE_STATISTICS
        s_data.stats.quad_count++;
    #endif
    }

    void Renderer2D::draw_text(const String& text, Ref<Font> font,
                               glm::vec3 pos, float scale, glm::vec4 color)
    {
        RKE_PROFILE_FUNCTION();

        auto* char_data{ static_cast<const stbtt_packedchar*>(font->get_char_data()) };
        float x{}, y{};
        for(const char* c{ text.raw() }; *c; c++) // should be safe with String
        {
            stbtt_aligned_quad quad{};
            stbtt_GetPackedQuad(char_data,
                font->get_atlas_size(), font->get_atlas_size(),
                static_cast<int>(*c - 32), &x, &y, &quad, 0);

            if(*c == ' ') continue;

            static QuadProps props{};
            props.position = {
                ( (quad.x0 + quad.x1) * (scale / font->get_font_size()) / 2.0f) + pos.x,
                (-(quad.y0 + quad.y1) * (scale / font->get_font_size()) / 2.0f) + pos.y, // flip manully
                pos.z
            };
            props.size = {
                -(quad.x1 - quad.x0) * (scale / font->get_font_size()), // flip manully
                 (quad.y1 - quad.y0) * (scale / font->get_font_size())
            };
            props.color   = color;
            props.texture = font->get_font_atlas().get();

            props.uv_coords[0] = { quad.s1, quad.t1 };
            props.uv_coords[1] = { quad.s0, quad.t1 };
            props.uv_coords[2] = { quad.s0, quad.t0 };
            props.uv_coords[3] = { quad.s1, quad.t0 };

            props.is_font = true;

            draw_quad(props);
        }
    }

// private
    void Renderer2D::start_batch()
    {
        RKE_PROFILE_FUNCTION();

        // refresh the huge data buffer
        auto handle{ Application::get()
            .get_window_lib()->get_current_context().get_integral() };
        CORE_ASSERT(s_data.context_data.count(handle),
            u8"Renderer2D: Drawing on an unregistered context!");
        auto& data{ s_data.context_data.at(handle) };
        data.index_count = 0;
        s_data.texture_slot_index = 1; // set to the head

        if(s_data.quad_vertex_ptr != nullptr) 
        {
            data.vbo->unmap();
            s_data.quad_vertex_ptr = nullptr;
            CORE_ERROR(u8"Renderer2D: VBO was explicitly "
                u8"unmapped before re-mapping. Check flush logic!");
        }
        s_data.quad_vertex_ptr = reinterpret_cast<QuadVertexProps*>
            (data.vbo->map(GBuffer::Access::Write));
        CORE_ASSERT(s_data.quad_vertex_ptr, u8"Renderer2D: Failed to map vertex buffer!");
    }

    void Renderer2D::flush()
    {
        RKE_PROFILE_FUNCTION();

        auto handle{ Application::get()
            .get_window_lib()->get_current_context().get_integral() };
        CORE_ASSERT(s_data.context_data.count(handle),
            u8"Renderer2D: Drawing on an unregistered context!");
        auto& data{ s_data.context_data.at(handle) };

        if(s_data.quad_vertex_ptr)
        {
            data.vbo->unmap();
            s_data.quad_vertex_ptr = nullptr;
        }
        if(data.index_count == 0) return;

        // bind textures
        for(uint32 i{}; i < s_data.texture_slot_index; i++)
            s_data.texture_slots[i]->bind
                (static_cast<uint32>(BindingPoint::Sampler2D_0) + i);

        data.vao->bind();
        RenderCommand::draw_indexed(data.index_count);
        data.vao->unbind();
    #ifdef RKE_ENABLE_STATISTICS
        s_data.stats.drawcall_count++;
    #endif
    }
#ifdef RKE_ENABLE_STATISTICS
    Renderer2D::Statistics& Renderer2D::get_stats() { return s_data.stats; }
    void Renderer2D::reset_stats() { s_data.stats = {}; }
#else
    Renderer2D::Statistics& Renderer2D::get_stats() { return s_data.stats; }
    void Renderer2D::reset_stats() {}
#endif
}
