module;

#include <map>
#include "rke_macros.h"

export module PanelRegistry;

import String;
import Types;
import Path;
import Panel;
import HeapManager;
import ConfigProxy;

export namespace rke
{
    class RKE_API PanelRegistry
    {
    public:
        friend class DockSpace;

        struct Attrib
        {
            bool always_on{ false };
            bool dont_block_when_hovered{ false };
            bool dont_block_when_focused{ false };
        };

        PanelRegistry() = default;
        ~PanelRegistry() = default;

        PanelRegistry(const PanelRegistry&) = delete;
        PanelRegistry& operator=(const PanelRegistry&) = delete;
        PanelRegistry(PanelRegistry&&) = delete;
        PanelRegistry& operator=(PanelRegistry&&) = delete;

        void register_panel(Panel* handle, Attrib attrib = {});
        void unregister_panel(Panel* handle);
    private:
        void serialize_to(ConfigDocument& proxy);
        void deserialize_from(ConfigReader& reader);

        void render_all();
        void render_switches_menubar();
    private:
        std::map<String, bool> config_{};
        std::map<Panel*, Attrib> attribs_{};
    };
}
