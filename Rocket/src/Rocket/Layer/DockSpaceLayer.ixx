module;

#include <string>
#include <filesystem>

export module DockSpaceLayer;

import String;
import Path;
import Layer;
import DockSpace;
import Event;
import Window;

export namespace rke
{
    class DockSpaceLayer : public Layer
    {
    public:
        friend class Application;

        DockSpaceLayer(String name, Window* owner, Path config_path);
        ~DockSpaceLayer() override = default;

        void on_event(Event& e) override;
        void on_update(float dt) override;
        void on_render() override;

        bool should_block_mouse() override;
        bool should_block_keyboard() override;
    private:
        DockSpace dockspace_;
    };
}
