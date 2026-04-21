module;
module PanelRegistry;

import Log;

namespace rke
{
    PanelRegistry::~PanelRegistry()
    {
        for(const auto& panel_attrib : panels_)
        {
            Panel* handle{ panel_attrib.handle };
            if(!handle) continue;
            if(panel_attrib.with_switch)
                panel_switches_[handle->get_name()] = handle->enabled();
        }
    }

    void PanelRegistry::set_switches_load_from(Path path)
        { panel_switches_.load_from(std::move(path)); }

    void PanelRegistry::register_panel(Panel* panel, bool with_switch,
                                       std::function<bool()> callback)
    {
        if(!panel) {
            CORE_ERROR(u8"PanelRegistrt: Panel invalid!");
            return;
        }
        panels_.emplace_back(panel, with_switch, std::move(callback));
        if(with_switch) {
            if(panel_switches_.has(panel->get_name()))
                panel->opened_ = panel_switches_.at(panel->get_name());
        }
    }

    void PanelRegistry::unregister_panel(Panel* panel)
    {
        std::erase_if(panels_, [panel](const auto& item)
            { return item.handle == panel; });
    }

    void PanelRegistry::on_imgui_render()
    {
        for(const auto& panel_attrib : panels_)
        {
            if(panel_attrib.render_condition_callback &&
              !panel_attrib.render_condition_callback()) continue;

            Panel* handle{ panel_attrib.handle };
            if(handle && handle->enabled()) handle->on_imgui_render();
        }
    }

    void PanelRegistry::render_switches_menubar()
    {
        for(auto& panel_attrib : panels_) {
            Panel* handle{ panel_attrib.handle };
            if(handle && panel_attrib.with_switch)
                ImGui::MenuItem(handle->get_name().raw(), "", &handle->opened_);
        }
    }
}
