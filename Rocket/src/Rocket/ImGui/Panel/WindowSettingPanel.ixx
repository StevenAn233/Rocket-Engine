module;

#include <format>
#include <utility>
#include "rke_macros.h"
namespace rke { class Window; }

export module WindowSettingPanel;

import Types;
import String;
import Panel;

export namespace rke
{
    class WindowSettingPanel : public Panel
    {
    public:
        WindowSettingPanel(String name, Window* owner);
    private:
        RKE_API void on_imgui_render() override;
        void update_smoothed_fps();
    private:
        Window* owner_{};
        double smoothed_fps_;
    };
}
