module;

#include <utility>
#include "rke_macros.h"

export module WindowSettingPanel;

import String;
import Panel;
import Window;

export namespace rke
{
    class RKE_API WindowSettingPanel : public Panel
    {
    public:
        WindowSettingPanel(String name) : Panel(std::move(name)) {}

        void set_context(Window* window) { context_ = window; }
        void on_imgui_render() override;
    private:
        Window* context_{};
    };
}
