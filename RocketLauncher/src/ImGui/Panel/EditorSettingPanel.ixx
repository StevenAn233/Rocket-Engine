module;
export module EditorSettingPanel;

import rke;
import Gizmo;
import OutlineEffect;

export namespace rke
{
    class EditorSettingPanel : public Panel
    {
    public:
        EditorSettingPanel(String name) : Panel(std::move(name)) {}
        ~EditorSettingPanel();

        const Path& get_last_proj_path() const { return last_proj_path_; }
        void on_imgui_render() override;

        void load_from(Path filepath);

        void set_outline_samples(uint32 samples);
        void set_last_proj_path (Path path);
        void set_selected_handle(OutlineEffect* handle) { selected_ = handle; }
        void set_hovering_handle(OutlineEffect* handle) { hovering_ = handle; }

        OutlineEffect* get_selected() const { return selected_; }
        OutlineEffect* get_hovering() const { return hovering_; }
        bool  selected_enabled_editor() const { return selected_enabled_editor_; }
        bool  hovering_enabled_editor() const { return hovering_enabled_editor_; }

        void on_viewport_resized(uint32 w, uint32 h);

        Gizmo::Mode get_gizmo_mode() const { return gizmo_mode_; }
        void set_gizmo_mode(Gizmo::Mode mode) { gizmo_mode_ = mode; }
    private:
        Path filepath_{}; // for saving

        Path last_proj_path_{};
        float font_scale_{ 1.0f };

        OutlineEffect* selected_{};
        OutlineEffect* hovering_{};
        bool selected_enabled_editor_{ true };
        bool hovering_enabled_editor_{ true };

        Gizmo::Mode gizmo_mode_{ Gizmo::Mode::Translate };
    };
}
