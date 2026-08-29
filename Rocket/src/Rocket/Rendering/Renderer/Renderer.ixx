module;

#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include "rke_macros.h"
namespace rke { class Window; }

export module Renderer;

import Types;
import Font;
import Mesh;
import Camera;
import HeapManager;
import Texture;
import Shader;
import String;
import NativeWindow;
import Buffers;
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
        struct VertexProps
        {
            glm::vec4 position{ 0.0f };
            glm::vec4 color{ 0.0f };
            glm::vec2 uv{ 0.0f };

            int tex_id{ 0 };
            int is_tex_grey{ 0 };
            int entity_id{-1 };
        };

        struct CameraData { glm::mat4 view_proj{ 1.0f }; };

        struct ContextData
        {
            Scope<VertexArray> vao{};
            Ref<VertexBuffer > vbo{}; // a huge vbo(for one context)
            Ref<IndexBuffer  > ibo{}; // a huge ibo(for one context)
            Ref<UniformBuffer> ubo{};

            uint32 vertex_count{};
            uint32 index_count {};
            VertexProps* vertex_props_it{ nullptr }; // vbo data ptr
            uint32* index_it{ nullptr }; // ibo data ptr
        };

        struct RKE_API Statistics
        {
            uint32 cam_set_count {};
            uint32 drawcall_count{};
            uint32 vertex_count{};
            uint32 index_count {};
        };
    public:
        static constexpr uint32 max_vertices{ 1000000 };
        static constexpr uint32 max_indices { 1500000 };
        static constexpr uint32 max_texture_slots{ 32 };
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

        RKE_API void push(const Mesh* mesh, const Texture2D* texture, const RenderProps& props);
    
        static void init(); // requires main window context current

    #ifdef RKE_ENABLE_STATISTICS
        inline Statistics& get_stats() { return stats_; }
        inline void reset_stats() { stats_ = {}; }
    #endif
    private:
        void start_batch();
        void flush();
    private:
        Window* context_;

        std::array<const Texture2D*, max_texture_slots> texture_slots_{};
        uint32 texture_slot_index_{ 1 }; // 0 for default texture
        bool in_scene_{ false };

        ContextData data_{};
        Scope<Texture2D> default_texture_{};
    #ifdef RKE_ENABLE_STATISTICS
        Statistics stats_{};
    #endif
        static Scope<Shader> s_shader;
    };
}
