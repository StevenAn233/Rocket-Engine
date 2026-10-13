module;

#include <imgui.h>
#include <imgui_internal.h>

module ProjectSettingPanel;

namespace {
    void add_text_vertical(ImDrawList* draw_list, const char* text, ImVec2 pos, ImU32 text_color)
    {
        constexpr float angle{ 1.570796f }; // 90 deg
        static float cos_a{ std::cosf(angle) };
        static float sin_a{ std::sinf(angle) };

        int vert_start{ draw_list->VtxBuffer.Size };
        draw_list->AddText(pos, text_color, text); // horizontal first
        int vert_end  { draw_list->VtxBuffer.Size };

        // rotate
        ImGui::ShadeVertsTransformPos
            (draw_list, vert_start, vert_end, pos, cos_a, sin_a, pos);
    }
}

namespace rke
{
    void ProjectSettingPanel::on_imgui_render()
    {
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        Project* active{ app().get_project() };
        if(!active) { ImGui::Text("No Active Project"); goto end; }

        if(ImGui::BeginTabBar("##project_settings_tabs"))
        {
            if(ImGui::BeginTabItem("Config"))
            {
                layout::tree_node_branch<u8"Name">([this, active]()
                {
                    const String& name{ active->get_name() };
                    char name_buffer[256]{};
                    std::memcpy(name_buffer, name.raw(),
                        std::min(name.length(), sizeof(name_buffer) - 1));
                    if(ImGui::InputText("##tag", name_buffer, sizeof(name_buffer),
                        ImGuiInputTextFlags_EnterReturnsTrue))
                        active->set_name(String(str::to_char8(name_buffer)));
                });
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Physics"))
            {
                draw_layer_collision_matrix(active->get_physics_layers());
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Render"))
            {
                layout::tree_node_branch<u8"Samples">([&]()
                {
                    constexpr const char* opts[]
                        { "1x", "2x", "4x", "8x", "16x" };
                    int shift{ 0 };

                    uint32 samples{ active->get_config().samples };
                    while(samples = samples >> 1) ++shift;

                    if(ImGui::Combo("##aa", &shift, opts, static_cast<int>(std::size(opts))))
                        { active->set_samples(static_cast<uint8>(shift)); }
                });
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    end:
        ImGui::End();
        ImGui::PopID();
    }

    void ProjectSettingPanel::draw_layer_collision_matrix(PhysicsLayers& layers)
    {
        ImGui::Text("Layer Collision");
        
        ImGui::SameLine();
        if(ImGui::SmallButton("-"))
           layers.minus_showed_layer_count();
        ImGui::SameLine();
        if(ImGui::SmallButton("+"))
            layers.plus_showed_layer_count();

        ImGui::Spacing();

        constexpr float spacing{ 4.0f }; // both x and y

        uint8 count{ layers.get_showed_layer_count() };
        if(count == 0) return;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f   ));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing , ImVec2(0.0f, spacing));

        static float cell_size{ ImGui::GetFrameHeight() };
        // only depends on font_size and frame_padding, also the size of checkbox
        constexpr float label_width{ 110.0f + spacing };

        char name_buffer[64]{};
        for(uint8 row{}; row < count; row++)
        {
            const String& row_name{ layers.get_name(row) };
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x);

            std::memset(name_buffer, 0, sizeof(name_buffer));
            std::memcpy(name_buffer, row_name.raw(),
                std::min(row_name.size(), sizeof(name_buffer) - 1));
            ImGui::PushID(row);
            ImGui::SetNextItemWidth(label_width - spacing);
            if(ImGui::InputText("##Name", name_buffer, sizeof(name_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue))
                layers.set_name(row, String(str::to_char8(name_buffer)));
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x + label_width);

            for(uint8 col{}; col < count; col++)
            {
                if(col > row) continue;
                ImGui::PushID(row * count + col);

                bool if_collides{ layers.if_collides(row, col) };
                if(ImGui::Checkbox("##mtx", &if_collides))
                    layers.set_collision(row, col, if_collides);

                if(ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s <-> %s",
                        row_name.raw(), layers.get_name(col).raw());

                ImGui::PopID();
                ImGui::SameLine();
                ImGui::SetCursorPosX (
                    ImGui::GetWindowContentRegionMin().x +
                    label_width +
                    (col + 1) * (cell_size + spacing)
                );
            }
            ImGui::NewLine();
        }

        ImVec2 cursor_screen_pos{ ImGui::GetCursorScreenPos() };
        ImDrawList* draw_list{ ImGui::GetWindowDrawList() };
        ImU32 text_col{ ImGui::GetColorU32(ImGuiCol_Text) };
        float start_x{ cursor_screen_pos.x + label_width };
        float start_y{ cursor_screen_pos.y };
        for(uint8 i{}; i < count; i++)
        {
            const String& text{ layers.get_name(i) };
            float x_pos { start_x
                + (i * (cell_size + spacing))
                + ImGui::CalcTextSize(text.raw()).y
            }; // ^ Will be rotated, so use y(height) here
            float y_pos{ start_y };
            add_text_vertical(draw_list, text.raw(), ImVec2(x_pos, y_pos), text_col);
        }

        ImGui::PopStyleVar(2);
    }
}
