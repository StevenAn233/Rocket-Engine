module;
export module SceneLayer;

import rke;
import EditorCamera;

export namespace rke
{
#if 0
    class SceneLayer : public Layer
    {
    public:
        SceneLayer(String name, Window* owner)
            : Layer(std::move(name), owner) {}

        void on_attach() override;
        void on_detach() override;

        void on_update(float dt) override;
        void on_render() override;

        bool should_block_mouse() override { return true; }
        bool should_block_keyboard() override { return true; }
    private:
        bool editing() const { return scene_state_ == SceneState::Edit; }
        bool playing() const { return scene_state_ == SceneState::Play; }
    private:
        SceneState scene_state_{ SceneState::Edit };
        EditorCamera editor_cam_{};

        SceneRenderer main_renderer_{ math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };

        int hovering_id_{ -1 };
        Ref<Scene> current_scene_{};
        Ref<Scene> origin_current_scene_{}; // For play/edit shifting
    };
# endif
}
