module;
export module EditorLayer;

import rke;
import OutlineEffect;
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
        friend class EditorSettingPanel;

        EditorLayer(String name, Window* owner);
        ~EditorLayer() override {};

        void on_attach() override;
        void on_detach() override;

        void on_event(Event& e) override;
        void on_update(float dt) override;
        void on_render() override;

        bool should_block_mouse() override;
        bool should_block_keyboard() override;
    private:
    // TO MODIFY
        void new_project();
        void open_project(const Window& window);
        void save_project();
    // TO MODIFY

        void on_runtime_start();
        void on_runtime_stop();

        Scope<Scene> load_scene_from(const Path& path);
        bool load_scene_edit_from(const Path& path); // Whole path given
        bool load_scene_edit(const String& name); // Project scene-dir / name.rkscene
        void clear_scene_edit();
        
        void attach_scene(Scene* scene);
        void entity_right_click_popup_content(Scene* scene);

        bool on_key_pressed(KeyPressedEvent& e);
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e);
        bool on_project_loaded(ProjectLoadedEvent& e);

        bool editing() const { return scene_edit_ && !scene_test_; }
        bool testing() const { return scene_edit_ &&  scene_test_; }
        Scene* current_scene();
    private:
        EditorCamera editor_cam_{};

        SceneRenderer main_renderer_{ math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        SceneRenderer cam_renderer_ { math::srgb_to_linear
            (glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        OutlineEffect* hovering_outline_{};
        OutlineEffect* selected_outline_{};

        int hovering_id_{ -1 };
        Path scene_edit_path_{};
        Scope<Scene> scene_edit_{};
        Scope<Scene> scene_test_{}; // A copy of scene_edit_; Temporary.
        SceneSerializer scene_serializer_{};

        const Texture2D* main_output_{};
        const Texture2D* cam_output_ {};

    // Panels
        Scope<EditorSettingPanel> editor_setting_panel_{};
        Scope<ContentBrowserPanel> content_browser_panel_{};

        Toolbar toolbar_{ u8"Toolbar" };
        Viewport main_viewport_{ u8"Main Viewport" };
        Viewport cam_viewport_{ u8"Camera Viewport" };
        SceneHierarchyPanel scene_hierarchy_panel_{ u8"Scene Hierarchy" };
        ProjectSettingPanel project_setting_panel_{ u8"Project Settings" };
    // Modals
        ProjectCreatingModal project_creating_modal_{ u8"Create New Project" };
        bool to_create_new_proj_{ false };
        bool in_main_viewport_dragging_{ false };
    };
}
