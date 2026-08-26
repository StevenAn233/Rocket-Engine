module;

#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include "rke_macros.h"
namespace rke { class Window; }

export module Renderer2D;

import Types;
import Font;
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
    class Renderer2D
    {
    public:
        struct RKE_API QuadProps
        {
            glm::mat4 transform;
            glm::vec4 color; // sRGB(need to linearlize)
            std::array<glm::vec2, 4> uv_coords;

            float tiling_factor{ 1.0f };
            Texture2D* texture{};
            bool make_tex_gray{ false };
            bool is_font{ false };
            int entity_id{ -1 };
        };

        struct QuadVertexProps // within VertexBuffer
        {
            glm::vec3 position { 0.0f };
            glm::vec4 color	   { 0.0f };
            glm::vec2 uv_coord { 0.0f };
            float tiling_factor{ 1.0f };

            int tex_id	   { 0 };
            int if_tex_grey{ 0 };
            int is_font	   { 0 };
            int entity_id  {-1 };
        };

        struct CameraData { glm::mat4 view_proj{ 1.0f }; };

        struct ContextData
        {
            Scope<VertexArray> vao{};
            Ref<VertexBuffer > vbo{}; // a huge vbo(for one context)
            Ref<IndexBuffer  > ibo{}; // a huge ibo(for one context)
            Ref<UniformBuffer> ubo{};

            CameraData camera_data{};
            uint32 index_count{};
        };

        struct RKE_API Statistics
        {
            uint32 cam_set_count {};
            uint32 drawcall_count{};
            uint32 quad_count	 {};

            uint32 vertex_count() const { return quad_count * 4; }
            uint32 index_count () const { return quad_count * 6; }
        };
    public:
        static constexpr uint32 max_quads{ 500000 };
        static constexpr uint32 max_vertices{ max_quads * 4 };
        static constexpr uint32 max_indices { max_quads * 6 };
        static constexpr uint32 max_texture_slots{ 32 };

        static constexpr std::array<glm::vec4, 4> quad_vertex_pos
        {
            glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
            glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f)
        };
    public:
        Renderer2D(Window* context);
        ~Renderer2D();

        Renderer2D(const Renderer2D&) = delete;
        Renderer2D& operator=(const Renderer2D&) = delete;
        Renderer2D(Renderer2D&&) = delete;
        Renderer2D& operator=(Renderer2D&&) = delete;

        RKE_API void begin_camera(const glm::mat4& view_projection);
        RKE_API void begin_scene();
        RKE_API void end_scene();

        RKE_API void draw_quad(const QuadProps& props);
        RKE_API void draw_text(const String& text,
            Font& font, glm::vec3 pos, float scale = 1.0f,
            glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
    
        static void init(); // requires a current context

    #ifdef RKE_ENABLE_STATISTICS
        inline Statistics& get_stats() { return stats_; }
        inline void reset_stats() { stats_ = {}; }
    #endif
    private:
        void start_batch();
        void flush();
    private:
        Window* context_;

        std::array<Texture2D*, max_texture_slots> texture_slots_{};
        uint32 texture_slot_index_{ 1 }; // 0 for default texture
        bool in_scene_{ false };

        Scope<ContextData> context_data_{};
        Scope<Texture2D> default_texture_{};
        QuadVertexProps* quad_vertex_ptr_{ nullptr };

        Statistics stats_{};

        static Scope<Shader> s_shader;
    };
}
