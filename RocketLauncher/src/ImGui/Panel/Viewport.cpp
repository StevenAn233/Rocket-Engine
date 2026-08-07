module;
module Viewport;

namespace rke
{
    Viewport::Viewport(String name, TargetGetter getter)
        : Panel(std::move(name)), getter_(std::move(getter))
    { CORE_ASSERT(getter_, u8"Viewport: Target getter null!"); }

    void Viewport::on_imgui_render()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(get_name().raw());

        refresh_state();

        if(const Texture2D* target{ getter_() }) {
            ImGui::Image(ImTextureRef(static_cast<ImTextureID>(target->get_renderer_id())),
                std::bit_cast<ImVec2>(get_size()), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        } else {
            ImVec2 cursor_pos{ ImGui::GetCursorScreenPos() };
            ImVec2 max_pos{ (cursor_pos.x + get_size().x), (cursor_pos.y + get_size().y) };

            ImGui::GetWindowDrawList()->AddRectFilled(cursor_pos, max_pos, IM_COL32(0, 0, 0, 255));

            if(get_size().x > 0 && get_size().y > 0)
            {
                ImGui::SetWindowFontScale(2.0f / ImGui::GetIO().FontGlobalScale);
                const char* text{ "No Target" };
                ImVec2 text_size{ ImGui::CalcTextSize(text) };
                ImVec2 text_pos { cursor_pos.x + (get_size().x - text_size.x) * 0.5f,
                                  cursor_pos.y + (get_size().y - text_size.y) * 0.5f };
                ImGui::GetWindowDrawList()->AddText(text_pos, IM_COL32(100, 100, 100, 255), text);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::InvisibleButton("##EmptyViewport", std::bit_cast<ImVec2>(get_size()));
            }
        }
        ImGui::PopStyleVar();

        if(callback_) callback_(*this);
        ImGui::End();
    }   
}
