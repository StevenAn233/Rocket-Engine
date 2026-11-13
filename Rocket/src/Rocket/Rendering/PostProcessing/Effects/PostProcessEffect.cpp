module;
module PostProcessEffect;

import Log;

namespace rke
{    
    bool PostProcessEffect::is_enabled() const { return enabled_; }

    void PostProcessEffect::serialize_to(ConfigWriter& writer) const
    {
        writer.begin_map(u8"Para");
        writer.write(u8"Enabled", enabled_);
        on_serialize(writer);
        writer.end_map();
    }

    void PostProcessEffect::deserialize_from(const ConfigReader& reader)
    {
        auto para{ reader.get_child(u8"Para") };
        if(!para || !para->is_map())
        {
            CORE_ERROR(u8"PostProcessEffect: Para reader invalid!");
            return;
        }
        enabled_ = para->get_at(u8"Enabled", true);
        on_deserialize(*para);
    }

    void PostProcessEffect::on_imgui_render()
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()
            + ImGui::GetContentRegionAvail().x
            - ImGui::GetFrameHeight()); // size of checkbox
        ImGui::Checkbox("##enabled", &enabled_);
    }
}
