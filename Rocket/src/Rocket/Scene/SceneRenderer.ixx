module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module SceneRenderer;

import Types;
import Scene;
import HeapManager;
import Texture;
import FrameBuffer;

import Shader;
import PostProcessor;
import PostProcessEffect;
import MathUtils;

namespace rke { struct Renderable; }

export namespace rke
{
    class RKE_API SceneRenderer
    {
    public:
        SceneRenderer(glm::vec4 clear_col);
        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&&) = delete;

        inline void set_samples(uint32 samples) { scene_fbo_->set_samples(samples); }

        void add_effect(Scope<PostProcessEffect> effect);
        const Texture2D* render(const Scene* scene, const glm::mat4& vp, glm::vec3 pos);
        const Texture2D* render(const Scene* scene, Entity camera);

        void on_viewport_resized(uint32 w, uint32 h);

        int get_hovering_id(int mouse_x, int mouse_y);
        void clean_up();
    private:
        struct Renderable
        {
            uint32 entity{ static_cast<uint32>(-1) };
            int layer{};
            float distance_sqr{};

            bool operator<(const Renderable& other) const
            {
                if(layer != other.layer) // judge by rendering layer first
                    return layer < other.layer;
                return distance_sqr > other.distance_sqr;
            }
        };

        void draw_renderable(const Scene* scene, const Renderable& renderable);
        void render_scene(const Scene* scene, const glm::mat4& view_projection, glm::vec3 cam_postion);
    private:
        glm::vec4 clear_color_;
        PostProcessor post_processor_{ clear_color_ };

        std::vector<Renderable> opaque_queue_{};
        std::vector<Renderable> cutout_queue_{};
        std::vector<Renderable> transparent_queue_{};

        Scope<FrameBuffer> scene_fbo_{};
    };
}
