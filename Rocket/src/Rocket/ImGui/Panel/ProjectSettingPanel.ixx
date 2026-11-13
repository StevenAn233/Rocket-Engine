module;

#include <utility>
#include "rke_macros.h"

export module ProjectSettingPanel;

import Panel;
import String;
import Project;
import PhysicsLayers;

export namespace rke
{
    class RKE_API ProjectSettingPanel : public Panel
    {
    public:
        ProjectSettingPanel(String name) : Panel(std::move(name)) {}
        inline void set_context(Project* context) { context_ = context; }
    private:
        void on_imgui_render() override;
        void draw_layer_collision_matrix(PhysicsLayers& layers);
        void effect_popup();
    private:
        Project* context_{};
    };
}
