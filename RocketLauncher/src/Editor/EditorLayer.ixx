module;
export module EditorLayer;

import rke;
import EditorCamera;

import SceneHierarchyPanel;
import WindowSettingPanel;
import EditorSettingPanel;
import ContentBrowserPanel;
import ProjectSettingPanel;
import ApplicationPanel;
import WindowSettingPanel;
import ProjectCreatingModal;
import Viewport;
import Toolbar;
import DockSpace;

namespace rke {
    enum class SceneState : uint32 { Edit = 0, Play };
}

export namespace rke
{
    class EditorLayer : public Layer
    {
    public:
        EditorLayer(String window_title, String name);

        void on_attach() override;
        void on_detach() override;

        void on_update(float dt) override;
        void on_render() override;
        void on_imgui_render() override;

        bool should_block_mouse   () override { return true; }
        bool should_block_keyboard() override { return true; }
    private:
        void new_project ();
        void open_project(const Window* window);
        void open_project(const Path& rkproj_path);
        void save_project();

        void on_runtime_start();
        void on_runtime_stop ();

        bool on_key_pressed(KeyPressedEvent& e) override;
        bool on_mouse_scrolled(MouseScrolledEvent& e) override;
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e) override;

        void update_current_scene(Ref<Scene> scene);

        void entity_right_click_popup_content(Scene* scene);
    private:
        bool editing() const { return scene_state_ == SceneState::Edit; }
        bool playing() const { return scene_state_ == SceneState::Play; }
    private:
    // Editor
        EditorCamera editor_cam_{};

    // Scene
        SceneRenderer main_renderer_
            { math::srgb_to_linear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };
        SceneRenderer cam_renderer_
            { math::srgb_to_linear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) };

        int hovering_id_{ -1 };

        SceneState scene_state_{ SceneState::Edit };
        Ref<Scene> current_scene_{};
        Ref<Scene> origin_current_scene_{}; // For play/edit shifting

    // DockSpace
        DockSpace dockspace_{ u8"DockSpace" };

    // Panels
        ApplicationPanel	application_panel_	  { u8"Application"	     };
        WindowSettingPanel  window_setting_panel_ { u8"Window Settings"  };
        SceneHierarchyPanel scene_hierarchy_panel_{ u8"Scene Hierarchy"  };
        EditorSettingPanel  editor_setting_panel_ { u8"Editor Settings"  };
        ContentBrowserPanel content_browser_panel_{ u8"Content Browser"  };
        ProjectSettingPanel project_setting_panel_{ u8"Project Settings" };

        Toolbar toolbar_{ u8"Toolbar" };
        Viewport main_viewport_{ u8"Main Viewport"   };
        Viewport cam_viewport_ { u8"Camera Viewport" };

        PanelRegistry panel_registry_{};

        bool to_create_new_proj_{ false };
        bool in_main_viewport_dragging_{ false };

        const Texture2D* main_output_{};
        const Texture2D* cam_output_ {};

        glm::vec2 last_vp_size_{};
        glm::vec2 last_cd_size_{};
        
    // Modals
        ProjectCreatingModal project_creating_modal_{ u8"Create New Project" };
    };
}
