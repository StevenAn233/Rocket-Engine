module;
module PanelRegistry;

import Log;
import FileUtils;

namespace rke
{
    void PanelRegistry::register_panel(Panel* handle, Attrib attrib)
    {
        Panel& panel{ *handle };
        if(config_.contains(panel.get_name()))
            panel.on_ = config_[panel.get_name()];
        if(attribs_.contains(handle))
        {
            CORE_ERROR(u8"PanelRegistry: Panel already registered!");
            return;
        }
        attribs_.emplace(handle, attrib);
    }

    void PanelRegistry::serialize_to(ConfigDocument& proxy)
    {
        for(auto& [handle, _] : attribs_)
            config_[handle->get_name()] = handle->on();
        auto panels_map{ proxy.get_child(u8"Panels") };
        for(const auto& config : config_)
            panels_map->write(config.first, config.second);
    }

    void PanelRegistry::deserialize_from(ConfigReader& reader)
    {
        Scope<ConfigReader> panels{ reader.get_child(u8"Panels") };
        if(panels) panels->for_each
            ([this](String name, Scope<ConfigReader> panel_on)
                { config_[name] = panel_on->as<bool>(true); });
    }

    void PanelRegistry::render_all()
    {
        for(const auto& [handle, _] : attribs_)
        {
            Panel& panel{ *handle };
            if(panel.on() && !panel.hidden())
                panel.on_imgui_render();
        }
    }

    void PanelRegistry::render_switches_menubar()
    {
        for(auto& [handle, attrib]: attribs_)
        {
            if(attrib.always_on) continue;
            ImGui::MenuItem(handle->get_name().raw(), "", &(handle->on_));
        }
    }
}
