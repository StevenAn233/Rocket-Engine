module;
export module EditorSettingPanel;

import rke;
import Gizmo;
import OutlineEffect;
import EditorCamera;

export namespace rke
{
    class EditorSettingPanel : public Panel
    {
    public:
        EditorSettingPanel(String name, EditorCamera* cam)
            : Panel(std::move(name)), camera_(cam) {}
        ~EditorSettingPanel();

        void load_from(Path filepath);
        void set_outline_samples(uint32 samples);

        inline void set_selected_handle(OutlineEffect* handle) { selected_ = handle; }
        inline void set_hovering_handle(OutlineEffect* handle) { hovering_ = handle; }
        inline void set_gizmo_mode(Gizmo::Mode mode) { gizmo_mode_ = mode; }

        inline OutlineEffect* get_selected() const { return selected_; }
        inline OutlineEffect* get_hovering() const { return hovering_; }
        inline Gizmo::Mode get_gizmo_mode() const { return gizmo_mode_; }

        inline bool selected_enabled_editor() const { return selected_enabled_editor_; }
        inline bool hovering_enabled_editor() const { return hovering_enabled_editor_; }
    private:
        void on_imgui_render() override;
    private:
        Path filepath_{};

        float font_scale_{ 1.0f };
        EditorCamera* camera_{};
        OutlineEffect* selected_{};
        OutlineEffect* hovering_{};
        bool selected_enabled_editor_{ true };
        bool hovering_enabled_editor_{ true };

        Gizmo::Mode gizmo_mode_{ Gizmo::Mode::Translate };
    };
}
