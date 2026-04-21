module;
module Toolbar;

namespace rke
{
    IconButton::IconButton(String name, String id, Ref<Texture2D> icon,
                           std::function<void(IconButton*)> on_click,
                           std::function<bool()> is_enabled, bool visible)
        : name_(std::move(name))
        , id_(std::move(id)), icon_(icon)
        , on_click_(std::move(on_click))
        , is_enabled_(std::move(is_enabled))
        , visible_(visible) {}

    void IconButton::render(float size)
    {
        CORE_ASSERT(icon_, u8"IconButton: Icon invalid!");

        bool now_disabled{ !is_enabled_() };
        if(visible_ || !now_disabled)
        {
            if(now_disabled) ImGui::BeginDisabled();
            if(ImGui::ImageButton(id_.raw(),
                static_cast<ImTextureID>(icon_->get_renderer_id()),
                { size, size }, { 0.0f, 1.0f }, { 1.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }
            ))	{ if(on_click_) on_click_(this); }

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", name_.raw());
            if(now_disabled) ImGui::EndDisabled();
        }
    }

    void Toolbar::on_imgui_render()
    {
        static constexpr ImGuiWindowFlags TOOLBAR_FLAGS
        {   ImGuiWindowFlags_NoDecoration
          | ImGuiWindowFlags_NoScrollbar
          | ImGuiWindowFlags_NoScrollWithMouse
        };

        const auto& colors{ ImGui::GetStyle().Colors };
        ImVec4 hovered_col{ colors[ImGuiCol_ButtonHovered] };
        ImVec4 active_col { colors[ImGuiCol_ButtonActive ] };
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.000f, 0.000f, 0.000f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImVec4(hovered_col.x, hovered_col.y, hovered_col.z, 0.5f)
        );
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            ImVec4(active_col.x, active_col.y, active_col.z, 0.5f)
        );

        ImVec2 def_window_padding{ ImGui::GetStyle().WindowPadding };
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,     ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,	ImVec2(0.0f, 4.0f));

        ImGui::Begin(get_name().raw(), nullptr, TOOLBAR_FLAGS);

        float btn_size{ ImGui::GetContentRegionAvail().y };
        float start_offset{ (
            ImGui::GetWindowContentRegionMax().x
            - (icon_buttons_.size() * btn_size * 1.5f)
            + btn_size * 0.5f
        ) * 0.5f };
        ImGui::SameLine(start_offset);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, def_window_padding);
        for(auto& button : icon_buttons_)
        {
            button.render(btn_size);
            start_offset += btn_size * 1.5f;
            ImGui::SameLine(start_offset);
        }
        ImGui::PopStyleVar();

        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
    }

    void Toolbar::emplace_icon_button(String name, 
        Ref<Texture2D> icon, std::function<void(IconButton*)> on_click,
        std::function<bool()> is_enabled, bool visible)
    {
        Size count{ icon_buttons_.size() + 1 };
        icon_buttons_.emplace_back(std::move(name),
            String::format(u8"##{}@icon_button@{}", get_name(), count),
            icon, std::move(on_click),
            is_enabled ? std::move(is_enabled) : []() { return true; },
            visible);
    }
}
