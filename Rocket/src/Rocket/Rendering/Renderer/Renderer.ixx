module;

#include <array>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "rke_macros.h"
namespace rke { class Window; }

export module Renderer;

import Types;
import Mesh;
import Camera;
import HeapManager;
import GTexture;
import Shader;
import String;
import NativeWindow;
import GBuffers;
import VertexArray;

export namespace rke
{
    struct RKE_API RenderProps
    {
        glm::mat4 transform;
        glm::vec2 uv_offset;
        glm::vec2 uv_scale;
        glm::vec4 color; // sRGB(need to linearlize)

        bool make_tex_gray{ false };
        uint32 entity_id{ 0xFFFFFFFFu };
    };

    class Renderer
    {
    public:
        struct InstanceData
        {
            glm::mat4 transform; // 64B
            glm::vec4 color;     // 16B
            glm::vec2 uv_offset; // 8B
            glm::vec2 uv_scale;  // 8B
            int tex_id;
            int is_tex_grey;
            int entity_id;
            int pad; // 112B total, 16-byte aligned stride
        };

        struct CameraData { glm::mat4 view_proj{ 1.0f }; };

        struct RKE_API Statistics
        {
            uint32 cam_set_count {};
            uint32 drawcall_count{};
            uint32 instance_count{};
        };
    public:
        static constexpr uint32 max_instances{ 100000 };
        static constexpr uint32 max_gtex_slots{ 32 };
    public:
        Renderer(Window* context);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        RKE_API void begin_camera(const glm::mat4& view_projection);
        RKE_API void begin_scene();
        RKE_API void end_scene();

        RKE_API void push(const Mesh* mesh, const GTexture* gtex, const RenderProps& props);
    
        static void init(); // requires main window context current

    #ifdef RKE_ENABLE_STATISTICS
        inline Statistics& get_stats() { return stats_; }
        inline void reset_stats() { stats_ = {}; }
    #endif
    private:
        struct MeshGeometry
        {
            Scope<VertexArray> vao{}; // mesh vbo(divisor0) + ibo + instance vbo(divisor1)
            Ref<VertexBuffer > vbo{}; // local vertices
            Ref<IndexBuffer  > ibo{}; // indices
            uint32 index_count{};
        };

        struct MeshGeometryGroup
        {
            MeshGeometry* geometry{};
            uint32 inst_start{};
            uint32 inst_count{};
        };

        void start_current_group(const Mesh* mesh);
        void close_current_group();
        void add_current_group_instance_count();

        void start_batch();
        void flush();
        void present_all_groups(); // requires instance_vbo unmapped

        MeshGeometry* get_or_create_mesh_geometry(const Mesh* mesh);
        uint32 find_or_add_gtex_slot(const GTexture* gtex);
    private:
        Window* context_;

        Scope<GTexture2D> default_texture_{};
        std::array<const GTexture*, max_gtex_slots> gtex_slots_{};
        uint32 gtex_slot_index_{ 1 }; // 0 for default texture
        bool in_scene_{ false };

        Ref<UniformBuffer> camera_ubo_{}; // CameraData
        Ref<VertexBuffer> instance_vbo_{}; // InstanceDatas
        uint32 instance_count_{};
        InstanceData* instance_it_{ nullptr }; // vbo data ptr
        
        std::unordered_map<const Mesh*, MeshGeometry> mesh_geometries_{};
        std::vector<MeshGeometryGroup> mesh_geometry_groups_{};

        const Mesh* resolved_mesh_{ nullptr };
        MeshGeometryGroup current_group_{};

    #ifdef RKE_ENABLE_STATISTICS
        Statistics stats_{};
    #endif
        static Scope<Shader> s_shader;
    };
}
