module;
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

    struct LocalVertex
    {
        glm::vec4 position;
        glm::vec2 uv;
    }; // 24B

    static_assert(Renderer::max_gtex_slots >= 2,
        u8"Renderer: Texture slots must be more than 2!");
    static_assert(Renderer::max_instances >= 0,
        u8"Renderer: Max instances must be more than 0!");
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

        instance_vbo_ = VertexBuffer::create(max_instances * sizeof(InstanceData));
        camera_ubo_ = UniformBuffer::create(sizeof(CameraData));
    }

    Renderer::~Renderer() {}

    void Renderer::begin_camera(const glm::mat4& view_projection)
    {
        RKE_PROFILE_FUNCTION();

        camera_ubo_->bind(BindingPoint::UBO_Camera);
        CameraData cam_data{ view_projection };
        camera_ubo_->set_data(&cam_data, sizeof(CameraData));

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

    // capacity: instance buffer full or texture slots full
        if(instance_count_  >= max_instances
        || gtex_slot_index_ >= max_gtex_slots) { flush(); start_batch(); }

    // implicit mesh grouping
        if(mesh != resolved_mesh_)
        {
            close_current_group();
            start_current_group(mesh);
            resolved_mesh_ = mesh;
        }

        InstanceData& inst{ *instance_it_ };
        inst.transform   = props.transform;
        inst.color       = math::srgb_to_linear(props.color);
        inst.uv_offset   = props.uv_offset;
        inst.uv_scale    = props.uv_scale;
        inst.tex_id      = static_cast<int>(find_or_add_gtex_slot(gtex));
        inst.is_tex_grey = static_cast<int>(props.make_tex_gray);
        inst.entity_id   = static_cast<int>(props.entity_id);
        inst.pad         = 0;

        instance_it_++;
        instance_count_++;
        add_current_group_instance_count();

    #ifdef RKE_ENABLE_STATISTICS
        stats_.instance_count++;
    #endif
    }

    void Renderer::start_current_group(const Mesh* mesh)
    {
        CORE_ASSERT(!current_group_.geometry, u8"Renderer: Current group not empty!");
        current_group_.geometry = get_or_create_mesh_geometry(mesh);
        current_group_.inst_start = instance_count_;
    }

    void Renderer::close_current_group()
    {
        if(current_group_.geometry && current_group_.inst_count > 0)
            mesh_geometry_groups_.push_back(current_group_);
        current_group_ = {};
    }

    void Renderer::add_current_group_instance_count()
    {
        if(!current_group_.geometry) return;
        current_group_.inst_count++;
    }

    void Renderer::start_batch()
    {   
    // clear
        gtex_slot_index_ = 1; // 0 for default gtex
        resolved_mesh_ = nullptr;
        instance_count_ = 0;
        mesh_geometry_groups_.clear();
        current_group_ = {};
        
        instance_it_ = reinterpret_cast<InstanceData*>(instance_vbo_->map(GBuffer::Access::Write));
        CORE_ASSERT(instance_it_, u8"Renderer: Failed to map instance buffer!");
    }

    void Renderer::flush()
    {
        close_current_group();
        // unmap first: drawing from a mapped (non-persistent) buffer is undefined
        instance_vbo_->unmap();
        instance_it_ = nullptr;
        present_all_groups(); // draw + clear groups
    }

    void Renderer::present_all_groups()
    {
        if(mesh_geometry_groups_.empty()) return;

    // bind textures
        for(uint32 i{}; i < gtex_slot_index_; i++)
            gtex_slots_[i]->bind(static_cast<uint32>(BindingPoint::Sampler2D_0) + i);

        for(const auto& group : mesh_geometry_groups_)
        {
            group.geometry->vao->bind();
            app().render_command().draw_instanced
            (
                static_cast<int>(group.geometry->index_count),
                static_cast<int>(group.inst_count),
                static_cast<int>(group.inst_start)
            );
            group.geometry->vao->unbind();
        }

    #ifdef RKE_ENABLE_STATISTICS
        stats_.drawcall_count += static_cast<uint32>(mesh_geometry_groups_.size());
    #endif
        mesh_geometry_groups_.clear();
    }

    Renderer::MeshGeometry* Renderer::get_or_create_mesh_geometry(const Mesh* mesh)
    {
        if(!mesh) return nullptr;
        auto it{ mesh_geometries_.find(mesh) };
        if(it != mesh_geometries_.end()) return &(it->second);

        MeshGeometry& geo{ mesh_geometries_[mesh] };
        uint32 vc{ mesh->get_vertex_count() };
        uint32 ic{ mesh->get_index_count () };

        LocalVertex* local_vertices{ new LocalVertex[vc]{} };
        for(uint32 i{}; i < vc; i++)
        {
            local_vertices[i].position = *(mesh->get_position(i));
            local_vertices[i].uv = *(mesh->get_uv(i));
        }

        uint32* indices{ new uint32[ic]{} };
        for(uint32 i{}; i < ic; i++) indices[i] = *(mesh->get_index(i));

        geo.vbo = VertexBuffer::create(local_vertices, vc * sizeof(LocalVertex));
        geo.ibo = IndexBuffer ::create(indices, ic);
        geo.index_count = ic;

        delete[] local_vertices;
        delete[] indices;

        geo.vao = VertexArray::create();

        rke::GBufferLayout local_layout
        {
            { u8"a_local_pos", rke::GShaderDataType::Float4 },
            { u8"a_uv", rke::GShaderDataType::Float2 }
        };
        geo.vao->add_vbo(geo.vbo, local_layout); // binding 0, divisor 0

        rke::GBufferLayout instance_layout
        {
            { u8"a_transform"  , rke::GShaderDataType::Mat4 },
            { u8"a_color"      , rke::GShaderDataType::Float4 },
            { u8"a_uv_offset"  , rke::GShaderDataType::Float2 },
            { u8"a_uv_scale"   , rke::GShaderDataType::Float2 },
            { u8"a_tex_id"     , rke::GShaderDataType::Int },
            { u8"a_is_tex_grey", rke::GShaderDataType::Int },
            { u8"a_entity_id"  , rke::GShaderDataType::Int },
            { u8"a_pad"        , rke::GShaderDataType::Int }
        };
        geo.vao->add_vbo(instance_vbo_, instance_layout); // binding 1
        geo.vao->set_binding_divisor(1, 1);
        geo.vao->set_ibo(geo.ibo);

        return &geo;
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
