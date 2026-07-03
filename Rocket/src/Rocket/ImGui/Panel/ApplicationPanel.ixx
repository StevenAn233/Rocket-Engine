module;

#include <utility>

export module ApplicationPanel;

import Panel;
import String;

export namespace rke
{
    class ApplicationPanel : public Panel
    {
    public:
        ApplicationPanel(String name)
            : Panel(std::move(name)) {}  
        void on_imgui_render() override;
    };
}
