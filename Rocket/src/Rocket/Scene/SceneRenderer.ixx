module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module SceneRenderer;

import Types;
import Scene;
import HeapManager;
import Texture;
import GTexture;
import FrameBuffer;
import Window;
import PostProcessor;
import PostProcessEffect;
import MathUtils;
import AssetsManager;

export namespace rke
{
    class RKE_API SceneRenderer
    {
    public:
        SceneRenderer(Window* context, glm::vec4 col);
        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) = delete;

        inline void set_samples(uint32 samples) { scene_fbo_->set_samples(samples); }
        
        PostProcessEffect* push_effect(Scope<PostProcessEffect> effect);
        Scope<PostProcessEffect> pop_effect();
        void refresh_post_processor_shaders();
        const GTexture2D* render(const Scene* scene, const glm::mat4& vp, glm::vec3 pos);
        const GTexture2D* render(const Scene* scene, Entity camera);

        void on_viewport_resized(uint32 w, uint32 h);

        int get_hovering_id(int mouse_x, int mouse_y);
        void clean_up();
    private:
        void draw_entity(AssetsManager& manager, const Scene* scene, uint32 handle);
        void render_scene(const Scene* scene, const glm::mat4& view_projection, glm::vec3 cam_postion);
    private:
        struct Renderable
        {
            uint32 handle;
            int layer;
            float distance_sqr;

            bool operator<(const Renderable& other) const
            {
                if(layer != other.layer) // judge by rendering layer first
                    return layer < other.layer;
                return distance_sqr > other.distance_sqr;
            }
        };

        Window* context_;
        glm::vec4 clear_color_;
        PostProcessor post_processor_{ clear_color_ };
        std::vector<Renderable> transparent_queue_{};
        Scope<FrameBuffer> scene_fbo_{};
    };
}
