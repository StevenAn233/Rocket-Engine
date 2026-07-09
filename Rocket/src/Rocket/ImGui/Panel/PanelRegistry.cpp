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

    void PanelRegistry::unregister_panel(Panel* handle)
    {
        if(attribs_.contains(handle))
        {
            config_[handle->get_name()] = handle->on();
            attribs_.erase(handle);
        }
    }

    void PanelRegistry::serialize_to(ConfigWriter& writer)
    {
        for(auto& [handle, _] : attribs_)
            config_[handle->get_name()] = handle->on();
        writer.begin_map(u8"Panels");
        for(const auto& config : config_)
            writer.write(config.first, config.second);
        writer.end_map();
    }

    void PanelRegistry::deserialize_from(const ConfigReader& reader)
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
