module;

#include <array>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Renderer2D;

import Types;
import Font;
import Camera;
import HeapManager;
import Texture;
import String;

export namespace rke
{
    class Renderer2D
    {
    public:
        static void init();
        static void shutdown();

        static void register_context();

        static RKE_API void begin_camera(const glm::mat4& view_projection);

        static RKE_API void begin_scene();
        static RKE_API void end_scene  ();

        struct RKE_API QuadProps
        {
            glm::vec3 position{ 0.0f };
            glm::vec3 rotation{ 0.0f }; // in radian
            glm::vec2 size { 1.0f };
            glm::vec4 color{ 1.0f };	// sRGB(need to linearlize)
            std::array<glm::vec2, 4> uv_coords
            {
                glm::vec2(1.0f, 1.0f),
                glm::vec2(0.0f, 1.0f),
                glm::vec2(0.0f, 0.0f),
                glm::vec2(1.0f, 0.0f)
            };
            float tiling_factor{ 1.0f };
            Texture2D* texture{};
            bool make_tex_gray{ false };
            bool is_font	  { false };

            int entity_id{ -1 };
        };

        static RKE_API void draw_quad(const QuadProps& props);
        static RKE_API void draw_text(const String& text,
            Ref<Font> font, glm::vec3 position_, float scale = 1.0f,
            glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
    private:
        static void start_batch();
        static void flush();
    public:
        struct RKE_API Statistics
        {
            uint32 cam_set_count {};
            uint32 drawcall_count{};
            uint32 quad_count	 {};

            uint32 vertex_count() const { return quad_count * 4; }
            uint32 index_count () const { return quad_count * 6; }
        };

        static RKE_API Statistics& get_stats();
        static RKE_API void reset_stats();
    };
}
