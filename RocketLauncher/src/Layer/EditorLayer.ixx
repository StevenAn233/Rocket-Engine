module;
export module EditorLayer;

import rke;
import Toolbar;
import Viewport;
import EditorSettingPanel;
import SceneHierarchyPanel;
import ContentBrowserPanel;
import ProjectSettingPanel;
import ProjectCreatingModal;
import EditorCamera;

export namespace rke
{
    class EditorLayer : public Layer
    {
    public:
        EditorLayer(String name, Window* owner)
            : Layer(std::move(name), owner) {}

        void on_attach() override;
        void on_detach() override;

        void on_event(Event& e) override;
        void on_update(float dt) override;
        void on_render() override;

        bool should_block_mouse() override
            { return scene_state_ == SceneState::Edit; }
        bool should_block_keyboard() override
            { return scene_state_ == SceneState::Edit; }
    private:
        void new_project();
        void open_project(const Window* window);
        void open_project(const Path& rkproj_path);
        void save_project();

        void on_runtime_start();
        void on_runtime_stop();

        void update_current_scene(Ref<Scene> scene);
        void entity_right_click_popup_content(Scene* scene);

    // for on_event
        bool on_key_pressed(KeyPressedEvent& e);
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e);
    private:
        Window* get_owner();
        bool editing() const { return scene_state_ == SceneState::Edit; }
        bool playing() const { return scene_state_ == SceneState::Play; }
    private:
        SceneState scene_state_{ SceneState::Edit };
        EditorCamera editor_cam_{};

        SceneRenderer main_renderer_{ math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        SceneRenderer cam_renderer_ { math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };

        int hovering_id_{ -1 };
        Ref<Scene> current_scene_{};
        Ref<Scene> origin_current_scene_{}; // For play/edit shifting

        const Texture2D* main_output_{};
        const Texture2D* cam_output_ {};

    // Panels
        Toolbar toolbar_{ u8"Toolbar" };
        Viewport main_viewport_{ u8"Main Viewport" };
        Viewport cam_viewport_{ u8"Camera Viewport" };
        EditorSettingPanel editor_setting_panel_ { u8"Editor Settings", editor_cam_ };
        SceneHierarchyPanel scene_hierarchy_panel_{ u8"Scene Hierarchy" };
        ContentBrowserPanel content_browser_panel_{ u8"Content Browser" };
        ProjectSettingPanel project_setting_panel_{ u8"Project Settings" };
    // Modals
        ProjectCreatingModal project_creating_modal_
            { u8"Create New Project", get_owner() };
        bool to_create_new_proj_{ false };
        bool in_main_viewport_dragging_{ false };
    };
}
