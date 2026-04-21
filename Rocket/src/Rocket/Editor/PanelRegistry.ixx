module;

#include <vector>
#include <functional>
#include "rke_macros.h"

export module PanelRegistry;

import Path;
import Panel;
import PanelSwitches;

export namespace rke
{
    class RKE_API PanelRegistry
    {
    public:
        PanelRegistry() = default;
        ~PanelRegistry();

        void set_switches_load_from(Path path);

        void register_panel(Panel* panel, bool with_switch = true,
                            std::function<bool()> callback = nullptr);
        void unregister_panel(Panel* panel);

        void on_imgui_render();
        void render_switches_menubar();
    private:
        struct PanelAttrib
        {
            Panel* handle;
            bool with_switch;
            std::function<bool()> render_condition_callback;

            PanelAttrib(Panel* ptr = nullptr, bool judge = true,
                        std::function<bool()>&& callback = nullptr)
                : handle(ptr), with_switch(judge)
                , render_condition_callback(std::move(callback)) {}
        };

        std::vector<PanelAttrib> panels_{};
        PanelSwitches panel_switches_{}; // may remove
        // Only used to serialize/deserialize!!!
    };
}
