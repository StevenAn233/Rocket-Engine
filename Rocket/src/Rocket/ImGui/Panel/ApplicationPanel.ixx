module;

#include <utility>
#include "rke_macros.h"

export module ApplicationPanel;

import Panel;
import String;

export namespace rke
{
    class ApplicationPanel : public Panel
    {
    public:
        ApplicationPanel(String name) : Panel(std::move(name)) {}
    private:
        RKE_API void on_imgui_render() override;
    };
}
