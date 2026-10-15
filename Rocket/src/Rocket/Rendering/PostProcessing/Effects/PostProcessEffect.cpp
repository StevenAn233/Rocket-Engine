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
        { writer.write(u8"Enabled", enabled_inner_); }

    void PostProcessEffect::deserialize_from(const ConfigReader& reader)
        { enabled_inner_ = reader.get_at(u8"Enabled", true); }

    void PostProcessEffect::on_imgui_render()
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()
            + ImGui::GetContentRegionAvail().x
            - ImGui::GetFrameHeight()); // size of checkbox
        ImGui::Checkbox("##enabled", &enabled_inner_);
    }
}
