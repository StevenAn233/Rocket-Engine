module;

#include <utility>
#include <functional>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module DockSpace;

import String;
import Path;
import PanelRegistry;
import NativeWindow;

export namespace rke
{
    class RKE_API DockSpace
    {
    public:
        friend class DockSpaceLayer;

        DockSpace(String name, Path config_path, NativeWindow context);
        ~DockSpace();

        void render(glm::vec2 offset, glm::vec2 scale);
        PanelRegistry& get_panel_registry() { return panel_registry_; }
    private:
        bool mouse_blocking() const;
        bool keyboard_blocking() const;
    private:
        String name_;
        Path config_path_;
        uint32 flags_{};
        PanelRegistry panel_registry_{};
    };
}
