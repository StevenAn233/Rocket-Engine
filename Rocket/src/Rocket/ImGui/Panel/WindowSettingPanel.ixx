module;

#include <utility>
#include "rke_macros.h"

export module WindowSettingPanel;

import String;
import Panel;

export namespace rke
{
    class WindowSettingPanel : public Panel
    {
    public:
        friend class Window;
        WindowSettingPanel(String name) : Panel(std::move(name)) {}
    private:
        void on_imgui_render() override;
        void set_context(void* window) { context_ = window; }
    private:
        void* context_{};
    };
}
