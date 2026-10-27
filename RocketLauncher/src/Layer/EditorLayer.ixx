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
        friend class SceneHierarchyPanel;

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

        void attach_scene(Scene* scene);
        bool load_scene_edit(const String& name);
        void save_scene_edit();
        void clear_scene_edit();
        void unload_scene_edit();
        void reload_scene_edit();
        
        bool on_key_pressed(KeyPressedEvent& e);
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e);

        void attach_context(Project* project);
        void on_context_project_saved();
        void on_context_project_samples_set(uint32 samples);

        bool editing() const { return scene_edit_ && !scene_test_; }
        bool testing() const { return scene_edit_ &&  scene_test_; }
        Scene* current_scene();
    private:
        EditorCamera editor_cam_{};
        Ticker ticker_{ 60 }; // hard-coded, may modify

        static constexpr glm::vec4 clear_color{ 0.1f, 0.1f, 0.1f, 1.0f };
        SceneRenderer main_renderer_{ &get_owner(),
            math::srgb_to_linear(clear_color) };
        SceneRenderer cam_renderer_ { &get_owner(),
            math::srgb_to_linear(clear_color) };
        PostProcessor post_processor_{ u8"Post-Processor",
            math::srgb_to_linear(clear_color) };
        PostProcessor cam_post_processor_{ u8"Cam Post-Processor",
            math::srgb_to_linear(clear_color) };

        EntityHandle hovering_id_{ entity_handle_null };
        Project* context_{};
        Scene* scene_edit_{};
        Scope<Scene> scene_test_{}; // A copy of scene_edit_; Temporary.
        SceneSerializer scene_serializer_{};
        std::unordered_map<String, ViewProps> editor_cams_{};

        const GTexture2D* main_output_{};
        const GTexture2D* cam_output_ {};

    // Panels
        Scope<EditorSettingPanel> editor_setting_panel_{};
        Scope<ContentBrowserPanel> content_browser_panel_{};
        Scope<Viewport> main_viewport_{};
        Scope<Viewport> cam_viewport_{};

        Toolbar toolbar_{ u8"Toolbar" };
        SceneHierarchyPanel scene_hierarchy_panel_{ u8"Scene Hierarchy" };
        AnimationEditorPanel animation_editor_panel_{ u8"Animation Editor" };

        bool in_main_viewport_dragging_{ false };
        bool in_entity_popup_{ false };
    };
}
