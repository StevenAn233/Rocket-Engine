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

        inline bool should_block_mouse() override { return editing(); }
        inline bool should_block_keyboard() override { return editing(); }
    private:
    // TO MODIFY
        void new_project();
        void open_project(const Window& window);
        void save_project();
    // TO MODIFY

        void on_runtime_start();
        void on_runtime_stop();

        Scope<Scene> load_scene(const String& name);
        void on_scene_loaded(Scene* scene);
        void entity_right_click_popup_content(Scene* scene);

        bool on_key_pressed(KeyPressedEvent& e);
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e);
        bool on_project_loaded(ProjectLoadedEvent& e);
    private:
        Window* get_owner();
        bool editing() const { return scene_edit_ && !scene_test_; }
        bool testing() const { return scene_edit_ &&  scene_test_; }
    private:
        EditorCamera editor_cam_{};

        SceneRenderer main_renderer_{ math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        SceneRenderer cam_renderer_ { math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };

        int hovering_id_{ -1 };
        Scope<Scene> scene_edit_{};
        Scope<Scene> scene_test_{};
        SceneSerializer scene_serializer_{};

        const Texture2D* main_output_{};
        const Texture2D* cam_output_ {};

    // Panels
        Toolbar toolbar_{ u8"Toolbar" };
        Viewport main_viewport_{ u8"Main Viewport" };
        Viewport cam_viewport_{ u8"Camera Viewport" };
        EditorSettingPanel editor_setting_panel_ { u8"Editor Settings", &editor_cam_ };
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
