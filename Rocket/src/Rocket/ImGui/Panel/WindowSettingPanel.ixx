module;

#include <format>
#include <utility>
#include "rke_macros.h"
namespace rke { class Window; }

export module WindowSettingPanel;

import String;
import Panel;

export namespace rke
{
    class WindowSettingPanel : public Panel
    {
    public:
        friend class Window;
        WindowSettingPanel(String name)
            : Panel(String::format(u8"Window: '{}'", std::move(name))) {}
    private:
        RKE_API void on_imgui_render() override;
        void set_context(Window* window);
    private:
        Window* context_{};
    };
}
