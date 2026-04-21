module;
module PanelSwitches;

import Log;
import FileUtils;
import HeapManager;
import ConfigProxy;

namespace rke
{
    PanelSwitches::~PanelSwitches()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map(); // Start Root
        writer->begin_map(u8"Panels"); // Start Panels
        for(const auto& [name, enabled] : switches_)
            writer->write(name, enabled);
        writer->end_map(); // End Panels
        writer->end_map(); // End Root
        writer->push_to_file(filepath_);
    }

    void PanelSwitches::check_to_add(const String& key, bool value)
        { if(!has(key)) switches_[key] = value; }

    bool PanelSwitches::has(const String& key) const
        { return switches_.find(key) != switches_.end(); }

    bool PanelSwitches::at(const String& key) const
    {
        auto it{ switches_.find(key) };
        if(it != switches_.end()) return it->second;
        CORE_ERROR(u8"PanelSwitches: Key '{}' not found!", key);
        return true; // opened
    }

    void PanelSwitches::load_from(Path filepath)
    {
        filepath_ = std::move(filepath);
        if(!filepath_.exists()) {
            CORE_WARN(u8"PanelSwitches: File \'{}\' not found!", filepath_);
            return;
        }
        Scope<ConfigReader> reader{ ConfigReader::create(filepath_) };
        Scope<ConfigReader> panels{ reader->get_child(u8"Panels") };
        if(panels) panels->for_each_map (
            [this](String name, Scope<ConfigReader> panel)
                { switches_[name] = panel->as(true); });
    }
}
