module;
export module ProjectSettingPanel;

import rke;

export namespace rke
{
    class ProjectSettingPanel : public Panel
    {
    public:
        ProjectSettingPanel(String name) : Panel(std::move(name)) {}
    private:
        void on_imgui_render() override;
        void draw_layer_collision_matrix(PhysicsLayers& layers);
    };
}
