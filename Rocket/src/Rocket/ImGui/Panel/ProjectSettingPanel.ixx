module;

#include <utility>
#include "rke_macros.h"

export module ProjectSettingPanel;

import String;
import Panel;
import Project;
import PhysicsLayers;

export namespace rke
{
    class ProjectSettingPanel : public Panel
    {
    public:
        friend class EditorLayer;

        ProjectSettingPanel(String name) : Panel(std::move(name)) {}
        void set_aa(AntiAliasing aa_opt);
    private:
        RKE_API void on_imgui_render() override;
        void draw_layer_collision_matrix(PhysicsLayers& layers);
    };
}
