module;
export module DockSpaceLayer;

import rke;

export namespace rke
{
    class DockSpaceLayer : public Layer
    {
    public:
        friend class RocketLauncher;

        DockSpaceLayer(String name, Window* owner, Path config_path);
        ~DockSpaceLayer() override = default;

        void on_event(Event& e) override;
        void on_update(double dt) override;
        void on_render() override;

        bool should_block_mouse() override;
        bool should_block_keyboard() override;
    private:
        DockSpace dockspace_;
    };
}
