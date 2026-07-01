module;
module Viewport;

namespace rke
{
    void Viewport::on_imgui_render()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(get_name().raw());

    // MUST be between viewport begin/end
        viewport_size_ = ImGui::GetContentRegionAvail();

        viewport_mouse_.x = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
        viewport_mouse_.y = viewport_size_.y
            - (ImGui::GetMousePos().y - ImGui::GetWindowPos().y);

        is_hovered_ = ImGui::IsWindowHovered();
        is_focused_ = ImGui::IsWindowFocused();
    // ---

        if(render_target_id_) {
            ImGui::Image(ImTextureRef(static_cast<ImTextureID>(render_target_id_)),
                viewport_size_, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        } else {
            ImVec2 cursor_pos{ ImGui::GetCursorScreenPos() };
            ImVec2 max_pos{ cursor_pos.x + viewport_size_.x, cursor_pos.y + viewport_size_.y };
            // full viewport

            ImGui::GetWindowDrawList()->AddRectFilled(cursor_pos, max_pos, IM_COL32(0, 0, 0, 255));

            if(viewport_size_.x > 0 && viewport_size_.y > 0)
            {
                ImGui::SetWindowFontScale(2.0f / ImGui::GetIO().FontGlobalScale);
                const char* text{ "No Scene Loaded" };
                ImVec2 text_size{ ImGui::CalcTextSize(text) };
                ImVec2 text_pos { cursor_pos.x + (viewport_size_.x - text_size.x) * 0.5f,
                                  cursor_pos.y + (viewport_size_.y - text_size.y) * 0.5f };
                ImGui::GetWindowDrawList()->AddText(text_pos, IM_COL32(100, 100, 100, 255), text);
                ImGui::SetWindowFontScale(1.0f);

                ImGui::InvisibleButton("##EmptyViewport", viewport_size_);
            }
        }
        render_target_id_ = 0;

        ImGui::PopStyleVar();
        if(in_viewport_callback_) in_viewport_callback_(this);

        ImGui::End();
    }   
}
