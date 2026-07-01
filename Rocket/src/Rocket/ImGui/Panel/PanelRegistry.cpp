module;
module PanelRegistry;

import Log;
import ConfigProxy;
import FileUtils;

namespace rke
{
    PanelRegistry::PanelRegistry(Path config_path)
        : filepath_(std::move(config_path))
    {
        if(!filepath_.exists()) {
            CORE_WARN(u8"PanelRegistry: File \'{}\' not found!", filepath_);
            return;
        }
        Scope<ConfigReader> reader{ ConfigReader::create(filepath_) };
        Scope<ConfigReader> panels{ reader->get_child(u8"Panels") };
        if(panels) panels->for_each_map
            ([this](String name, Scope<ConfigReader> panel_stat)
                { config_[name] = panel_stat->as(true); });
    }

    PanelRegistry::~PanelRegistry()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map(); // Start Root
        writer->begin_map(u8"Panels"); // Start Panels
        for(const auto& config : config_)
            writer->write(config.first, config.second);
        writer->end_map(); // End Panels
        writer->end_map(); // End Root
        writer->push_to_file(filepath_);
    }

    void PanelRegistry::push(Attrib attrib)
    {
        Panel& panel{ *(attrib.handle) };
        if(config_.contains(panel.get_name()))
            panel.on_ = config_[panel.get_name()];
        attribs_.push_back(std::move(attrib));
    }

    void PanelRegistry::pop(Size count)
    {
        for(Size i{}; i < count; i++)
        {
            Attrib& attrib{ attribs_.back() };
            Panel& panel{ *(attrib.handle) };
            config_[panel.get_name()] = panel.on();
            attribs_.pop_back();
        }
    }

    void PanelRegistry::render_all()
    {
        for(const auto& attrib : attribs_)
        {
            if(attrib.cond_callback && !attrib.cond_callback()) continue;
            Panel& panel{ *(attrib.handle) };
            panel.render();
        }
    }

    void PanelRegistry::render_switches_menubar()
    {
        for(auto& attrib : attribs_)
        {
            if(attrib.with_switch)
            {
                Panel& panel{ *(attrib.handle) };
                ImGui::MenuItem(panel.get_name().raw(), "", &(panel.on_));
            }
        }
    }
}
