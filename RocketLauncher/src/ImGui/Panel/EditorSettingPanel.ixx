module;

namespace rke { class EditorLayer; }

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
        EditorSettingPanel(String name, EditorLayer* owner);
        ~EditorSettingPanel();

        void load_from(Path filepath);
        void set_outline_samples(uint32 samples);

        inline void set_gizmo_mode(gizmo::Mode mode) { gizmo_mode_ = mode; }
        inline gizmo::Mode get_gizmo_mode() const { return gizmo_mode_; }

        inline bool selected_enabled_editor() const { return selected_enabled_editor_; }
        inline bool hovering_enabled_editor() const { return hovering_enabled_editor_; }
    private:
        void on_imgui_render() override;
        OutlineEffect* hovering_outline();
        OutlineEffect* selected_outline();
    private:
        EditorLayer* owner_;
        Path filepath_{};

        float font_scale_{ 1.0f };
        bool selected_enabled_editor_{ true };
        bool hovering_enabled_editor_{ true };

        gizmo::Mode gizmo_mode_{ gizmo::Mode::Translate };
    };
}
