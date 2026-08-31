module Renderer;

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
import GShader;

namespace {
    using namespace rke;
    static_assert(Renderer::max_gtex_slots >= 2,
        u8"Renderer: Texture slots must be more than 2!");
    static_assert(Renderer::max_vertices >= 0,
        u8"Renderer: Max veritices must be more than 0!");
    static_assert(Renderer::max_indices >= 0,
        u8"Renderer: Max indices must be more than 0!");
}

namespace rke
{
    Scope<Shader> Renderer::s_shader{};

    void Renderer::init()
    {
        Path shader_path{ file::assets_dir() / u8"shaders" / u8"renderer.rkshdr" };
        s_shader = create_scope<Shader>(shader_path);
    }

    Renderer::Renderer(Window* context) : context_(context)
    {
        CORE_ASSERT(context_, u8"Renderer: Context window null!");

        uint32 pixel{ 0xFFFFFFFF };
        default_texture_ = GTexture2D::create
            (1, 1, GTexture::Format::RGBA8, &pixel, {}, {});
        gtex_slots_[0] = default_texture_.get();

        data_.vao = VertexArray ::create();
        data_.vbo = VertexBuffer::create(max_vertices * sizeof(VertexProps));
        // huge, empty vbo(only with size)
        data_.ubo = UniformBuffer::create(sizeof(CameraData));

        rke::GBufferLayout vertex_props_layout
        {
            { u8"a_position", rke::GShaderDataType::Float4 },
            { u8"a_color"   , rke::GShaderDataType::Float4 },
            { u8"a_uv_coord", rke::GShaderDataType::Float2 },

            { u8"a_tex_id"	   , rke::GShaderDataType::Int },
            { u8"a_is_tex_grey", rke::GShaderDataType::Int },
            { u8"a_entity_id"  , rke::GShaderDataType::Int }
        };
        data_.vao->add_vbo(data_.vbo, vertex_props_layout);
        data_.ibo = IndexBuffer::create(nullptr, max_indices);
        data_.vao->set_ibo(data_.ibo);
    }

    Renderer::~Renderer() {}

    void Renderer::begin_camera(const glm::mat4& view_projection)
    {
        RKE_PROFILE_FUNCTION();

        data_.ubo->bind(BindingPoint::UBO_Camera);
        CameraData cam_data{ view_projection };
        data_.ubo->set_data(&cam_data, sizeof(CameraData));

    #ifdef RKE_ENABLE_STATISTICS
        stats_.cam_set_count++;
    #endif
    }

    void Renderer::begin_scene()
    {
        in_scene_ = true;
        s_shader->get_gshader()->bind();
        start_batch();
    }

    void Renderer::end_scene()
    {
        flush();
        s_shader->get_gshader()->unbind();
        in_scene_ = false;
    }

    void Renderer::push(const Mesh* mesh, const GTexture* gtex, const RenderProps& props)
    {
        RKE_PROFILE_FUNCTION();
        if(!mesh) return;
        CORE_ASSERT(in_scene_, u8"Renderer: Can't push when not in scene!");

        uint32 vc{ mesh->get_vertex_count() };
        uint32 ic{ mesh->get_index_count () };
        if(vc >= max_vertices || ic >= max_indices) {
            CORE_ERROR(u8"Renderer: Not supported mesh; Way too huge!");
            return;
        }
        if(data_.vertex_count + vc >= max_vertices
        || data_.index_count  + ic >= max_indices
        || gtex_slot_index_ >= max_gtex_slots) { flush(); start_batch(); }

        uint32 gtex_index{ find_or_add_gtex_slot(gtex) };
        glm::vec4 linearlized{ math::srgb_to_linear(props.color) };

        for(uint32 i{}; i < vc; i++)
        {
            data_.vertex_props_it->position = props.transform * (*(mesh->get_position(i))); // to GPU?
            data_.vertex_props_it->color = linearlized; // to merge with vertex color
            data_.vertex_props_it->uv = props.uv_scale * (*(mesh->get_uv(i))) + props.uv_offset; // to GPU?

            data_.vertex_props_it->tex_id      = static_cast<int>(gtex_index);
            data_.vertex_props_it->is_tex_grey = static_cast<int>(props.make_tex_gray);
            data_.vertex_props_it->entity_id   = static_cast<int>(props.entity_id);
            data_.vertex_props_it++; // stride: VertexProps
        }

        uint32 base{ data_.vertex_count };
        for(uint32 i{}; i < ic; i++)
        {
            *(data_.index_it) = base + *(mesh->get_index(i));
            data_.index_it++;
        }
        data_.vertex_count += vc;
        data_.index_count  += ic;

    #ifdef RKE_ENABLE_STATISTICS
        stats_.vertex_count += vc;
        stats_.index_count  += ic;
    #endif
    }

    void Renderer::start_batch()
    {
        RKE_PROFILE_FUNCTION();

        data_.vertex_count = 0;
        data_.index_count = 0;
        gtex_slot_index_ = 1; // 0 for default gtex

        data_.vertex_props_it = reinterpret_cast<VertexProps*>
            (data_.vbo->map(GBuffer::Access::Write));
        data_.index_it = reinterpret_cast<uint32*>(data_.ibo->map(GBuffer::Access::Write));
        CORE_ASSERT(data_.vertex_props_it, u8"Renderer: Failed to map vertex buffer!");
        CORE_ASSERT(data_.index_it, u8"Renderer: Failed to map index buffer!");
    }

    void Renderer::flush()
    {
        RKE_PROFILE_FUNCTION();

        data_.vbo->unmap();
        data_.vertex_props_it = nullptr;
       
        data_.ibo->unmap();
        data_.index_it = nullptr;
        
        if(data_.index_count == 0) return;

    // bind textures
        for(uint32 i{}; i < gtex_slot_index_; i++)
            gtex_slots_[i]->bind(static_cast<uint32>(BindingPoint::Sampler2D_0) + i);

        data_.vao->bind();
        app().render_command().draw_indexed(data_.index_count);
        data_.vao->unbind();

    #ifdef RKE_ENABLE_STATISTICS
        stats_.drawcall_count++;
    #endif
    }

    uint32 Renderer::find_or_add_gtex_slot(const GTexture* gtex)
    {
        // find gtex id
        uint32 gtex_index{ 0 }; // white gtex(default)
        if(gtex) {
            bool found{ false };
            for(uint32 i{}; i < gtex_slot_index_; i++)
            {
                if(gtex_slots_[i] == gtex)
                {
                    gtex_index = i;
                    found = true; break;
                }
            }
            if(!found) {
                gtex_index = gtex_slot_index_;
                gtex_slots_[gtex_slot_index_] = gtex;
                gtex_slot_index_++;
            }
        }
        return gtex_index;
    }
}
