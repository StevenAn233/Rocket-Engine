module;
module PanelRegistry;

import Log;
import FileUtils;

namespace rke
{
    void PanelRegistry::register_panel(Attrib attrib)
    {
        Panel& panel{ *(attrib.handle) };
        if(config_.contains(panel.get_name()))
            panel.on_ = config_[panel.get_name()];
        attribs_.push_back(std::move(attrib));
    }

    void PanelRegistry::serialize_to(ConfigDocument& proxy)
    {
        for(auto& attrib : attribs_)
        {
            Panel& panel{ *(attrib.handle) };
            config_[panel.get_name()] = panel.on();
        }
        auto panels_map{ proxy.get_child(u8"Panels") };
        for(const auto& config : config_)
            panels_map->write(config.first, config.second);
    }

    void PanelRegistry::deserialize_from(ConfigReader& reader)
    {
        Scope<ConfigReader> panels{ reader.get_child(u8"Panels") };
        if(panels) panels->for_each
            ([this](String name, Scope<ConfigReader> panel_stat)
                { config_[name] = panel_stat->as(true); });
    }

    void PanelRegistry::render_all()
    {
        for(const auto& attrib : attribs_)
        {
            Panel& panel{ *(attrib.handle) };
            panel.render();
        }
    }

    void PanelRegistry::render_switches_menubar()
    {
        for(auto& attrib : attribs_)
        {
            if(!attrib.always_on)
            {
                Panel& panel{ *(attrib.handle) };
                ImGui::MenuItem(panel.get_name().raw(), "", &(panel.on_));
            }
        }
    }
}
