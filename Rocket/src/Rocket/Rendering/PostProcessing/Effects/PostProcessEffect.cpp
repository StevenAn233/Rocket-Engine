module;
module PostProcessEffect;

import Log;

namespace rke
{
    PostProcessEffect::PostProcessEffect(String name, std::function<bool()> extra)
        : name_(name.empty() ? u8"Untitled" : std::move(name))
        , enabled_extra_(std::move(extra))
        { if(!enabled_extra_) enabled_extra_ = []() { return true; }; }

    void PostProcessEffect::serialize_to(ConfigWriter& writer) const
    {
        writer.begin_map(get_name());
        writer.write(u8"Enabled", enabled_inner_);
        on_serialize(writer);
        writer.end_map();
    }

    void PostProcessEffect::deserialize_from(const ConfigReader& reader)
    {
        auto child_reader{ reader.get_child(get_name()) };
        if(!child_reader || !child_reader->is_map())
        {
            CORE_ERROR(u8"PostProcessEffect: Child reader invalid!");
            return;
        }
        enabled_inner_ = child_reader->get_at(u8"Enabled", true);
        on_deserialize(*child_reader);
    }

    void PostProcessEffect::on_imgui_render()
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()
            + ImGui::GetContentRegionAvail().x
            - ImGui::GetFrameHeight()); // size of checkbox
        ImGui::Checkbox("##enabled", &enabled_inner_);
    }
}
