module;
export module EditorLayer;

import rke;
import OutlineEffect;
import Toolbar;
import Viewport;
import EditorSettingPanel;
import SceneHierarchyPanel;
import ContentBrowserPanel;
import EditorCamera;

export namespace rke
{
    class EditorLayer : public Layer
    {
    public:
        friend class RocketLauncher;
        friend class EditorSettingPanel;

        EditorLayer(String name, Window* owner);
        ~EditorLayer() override {};

        void on_attach() override;
        void on_detach() override;

        void on_event(Event& e) override;
        void on_update(double dt) override;
        void on_render() override;

        bool should_block_mouse() override;
        bool should_block_keyboard() override;
    private:
        void on_runtime_start();
        void on_runtime_stop ();

        bool load_scene_edit(const String& name);
        void save_scene_edit();
        void clear_scene_edit();
        
        void attach_scene(Scene* scene);

        bool on_key_pressed(KeyPressedEvent& e);
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e);

        bool on_project_loaded(ProjectLoadedEvent& e);
        bool on_project_saved(ProjectSavedEvent& e);
        bool on_project_samples_set(ProjectSamplesSetEvent& e);

        bool editing() const { return scene_edit_ && !scene_test_; }
        bool testing() const { return scene_edit_ &&  scene_test_; }
        Scene* current_scene();
    private:
        EditorCamera editor_cam_{};
        Ticker ticker_{ 60 }; // hard-coded, may modify

        SceneRenderer main_renderer_{ &get_owner(),
            math::srgb_to_linear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        SceneRenderer cam_renderer_ { &get_owner(),
            math::srgb_to_linear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        OutlineEffect* hovering_outline_{};
        OutlineEffect* selected_outline_{};

        uint32 hovering_id_{ entity_id_null };
        Scene* scene_edit_{};
        Scope<Scene> scene_test_{}; // A copy of scene_edit_; Temporary.
        SceneSerializer scene_serializer_{};

        const GTexture2D* main_output_{};
        const GTexture2D* cam_output_ {};

    // Panels
        Scope<EditorSettingPanel> editor_setting_panel_{};
        Scope<ContentBrowserPanel> content_browser_panel_{};
        Scope<Viewport> main_viewport_{};
        Scope<Viewport> cam_viewport_{};

        Toolbar toolbar_{ u8"Toolbar" };
        SceneHierarchyPanel scene_hierarchy_panel_{ u8"Scene Hierarchy" };

        bool in_main_viewport_dragging_{ false };
        bool in_entity_popup_{ false };
    };
}
