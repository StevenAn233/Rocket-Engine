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

        inline void set_gizmo_mode(gizmo::Mode mode) { gizmo_mode_ = mode; }
        inline gizmo::Mode get_gizmo_mode() const { return gizmo_mode_; }
    private:
        void on_imgui_render() override;
    private:
        EditorLayer* owner_;
        Path filepath_{};

        float font_scale_{ 1.0f };
        gizmo::Mode gizmo_mode_{ gizmo::Mode::Translate };
    };
}
