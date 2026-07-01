module;

#include <vector>
#include <functional>
#include <unordered_map>
#include "rke_macros.h"

export module PanelRegistry;

import Types;
import Path;
import Panel;
import HeapManager;

export namespace rke
{
    class RKE_API PanelRegistry
    {
    public:
        struct Attrib
        {
            Panel* handle;
            bool with_switch{ true };
            std::function<bool()> cond_callback{};
        };

        PanelRegistry(Path config_path);

        PanelRegistry(const PanelRegistry&) = delete;
        PanelRegistry& operator=(const PanelRegistry&) = delete;
        PanelRegistry(PanelRegistry&&) = delete;
        PanelRegistry& operator=(PanelRegistry&&) = delete;

        ~PanelRegistry();

        void push(Attrib attrib);
        void pop(Size count = 1);
    
        void render_all();
        void render_switches_menubar();
    private:
        Path filepath_;
        std::unordered_map<String, bool> config_{};
        std::vector<Attrib> attribs_{};
    };
}
