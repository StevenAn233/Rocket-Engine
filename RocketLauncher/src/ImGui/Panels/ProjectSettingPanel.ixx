module;
export module ProjectSettingPanel;

import rke;
import FXAAEffect;

export namespace rke
{
    class ProjectSettingPanel : public Panel
    {
    public:
        ProjectSettingPanel(String name) : Panel(std::move(name)) {}

        void on_imgui_render() override;

        void set_on_samples_setting(std::function<void(uint32)> callback)
            { on_samples_setting_ = std::move(callback); }
        void set_fxaa_handle(FXAAEffect* handle) { fxaa_ = handle; }
        void refresh_aa_setting(); // call after loading project

        void on_viewport_resized(uint32 w, uint32 h);
    private:
        void draw_layer_collision_matrix(PhysicsLayers& layers);
        void apply_aa_setting(int aa_opt);
    private:
        FXAAEffect* fxaa_{};
        bool viewport_valid_for_fxaa_{ true };

        std::function<void(uint32)> on_samples_setting_{};
    };
}
