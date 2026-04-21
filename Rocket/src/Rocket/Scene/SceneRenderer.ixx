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
        SceneRenderer(glm::vec4 col =
            math::srgb_to_linear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&& ____) = delete;

        void add_effect(Scope<PostProcessEffect> effect)
            { post_processor_.add_effect(std::move(effect)); }

        const Texture2D* on_render(const Scene* scene, const glm::mat4& vp, glm::vec3 pos);
        const Texture2D* on_render_runtime(const Scene* scene); // uses master camera(in the scene)
        const Texture2D* cam_demo_render  (const Scene* scene, Entity cam_demo);
        void on_viewport_resized(uint32 w, uint32 h);

        int get_hovering_id(int mouse_x, int mouse_y);
        void clean_up() { // on_scene_changed
            scene_fbo_->clear_pbo();
            scene_fbo_->clear();
            post_processor_.clean_up();
        }

        void set_samples(uint32 samples) { scene_fbo_->set_samples(samples); }

        void set_cam_demo_target(Entity entity) { cam_demo_target_ = entity; }
        Entity get_cam_demo_target() const { return cam_demo_target_; }
        void cam_demo_validation_check() { cam_demo_target_.invalidate_if_unavailable(); }
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

        std::vector<Renderable> opaque_queue_{};
        std::vector<Renderable> cutout_queue_{};
        std::vector<Renderable> transparent_queue_{};

        Entity cam_demo_target_{}; // MAY MODIFY
        Ref<FrameBuffer> scene_fbo_{}; // has its ownership

        PostProcessor post_processor_{ clear_color_ };
    };
}
