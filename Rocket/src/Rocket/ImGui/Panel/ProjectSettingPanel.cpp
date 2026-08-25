module;

#include <imgui.h>
#include <imgui_internal.h>

module ProjectSettingPanel;

import Types;
import ProjectEvent;
import Application;
import Layout;

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
    void ProjectSettingPanel::set_aa(AntiAliasing aa_opt)
    {
        Project* project{ app().get_project() };
        if(project) project->get_config_mut().anti_aliasing = aa_opt;

        uint32 samples{ 1 };
        switch(aa_opt)
        {
        case AntiAliasing::MSAAx2:  samples = 2;  break;
        case AntiAliasing::MSAAx4:  samples = 4;  break;
        case AntiAliasing::MSAAx8:  samples = 8;  break;
        case AntiAliasing::MSAAx16: samples = 16; break;
        }
        
        ProjectSamplesSetEvent event{ u8"main", samples };
        app().send_event(event);
    }

    void ProjectSettingPanel::on_imgui_render()
    {
        Project* project{ app().get_project() };
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        if(!project) { ImGui::Text("No Active Project"); goto end; }

        if(ImGui::BeginTabBar("##project_settings_tabs"))
        {
            if(ImGui::BeginTabItem("Config"))
            {
                layout::tree_node_branch<u8"Name">([this]()
                {
                    const String& name{ app().get_project()->get_name() };
                    char name_buffer[256]{};
                    std::memcpy(name_buffer, name.raw(), sizeof(name_buffer) - 1);
                    if(ImGui::InputText("##tag", name_buffer, sizeof(name_buffer),
                        ImGuiInputTextFlags_EnterReturnsTrue))
                        app().get_project()->set_name(String(str::to_char8(name_buffer)));
                });
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Physics"))
            {
                draw_layer_collision_matrix(project->get_config_mut().physics_layers);
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Render"))
            {
                layout::tree_node_branch<u8"Anti-Aliasing">([&]()
                {
                    static const char* items[]
                    {
                        "Off",      // 0
                        "2x MSAA",  // 1
                        "4x MSAA",  // 2
                        "8x MSAA",  // 3
                        "16x MSAA", // 4
                        "FXAA"      // 5
                    };
                    int aa_opt{};
                    switch(project->get_config().anti_aliasing)
                    {
                    case AntiAliasing::Off:     aa_opt = 0; break;
                    case AntiAliasing::MSAAx2:  aa_opt = 1; break;
                    case AntiAliasing::MSAAx4:  aa_opt = 2; break;
                    case AntiAliasing::MSAAx8:  aa_opt = 3; break;
                    case AntiAliasing::MSAAx16: aa_opt = 4; break;
                    case AntiAliasing::FXAA:    aa_opt = 5; break;
                    }
                    if(ImGui::Combo("##aa", &aa_opt, items, static_cast<int>(std::size(items))))
                    {
                        AntiAliasing aa{ AntiAliasing::Off };
                        switch(aa_opt)
                        {
                        case 0: aa = AntiAliasing::Off;     break;
                        case 1: aa = AntiAliasing::MSAAx2;  break;
                        case 2: aa = AntiAliasing::MSAAx4;  break;
                        case 3: aa = AntiAliasing::MSAAx8;  break;
                        case 4: aa = AntiAliasing::MSAAx16; break;
                        case 5: aa = AntiAliasing::FXAA;    break;
                        }
                        set_aa(aa);
                    }
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
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f   ));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing , ImVec2(0.0f, spacing));

        uint8 count{ layers.get_showed_layer_count() };
        if(count == 0) return;

        static float cell_size{ ImGui::GetFrameHeight() };
        // only depends on font_size and frame_padding, also the size of checkbox
        constexpr float label_width{ 110.0f + spacing };

        ImDrawList* draw_list { ImGui::GetWindowDrawList() };
        ImU32 text_col { ImGui::GetColorU32(ImGuiCol_Text) };
        float font_size{ ImGui::GetFontSize() };

        char name_buffer[64]{};
        for(uint8 row{}; row < count; row++)
        {
            const String& row_name{ layers.get_name(row) };

            float text_w{ ImGui::CalcTextSize(row_name.raw()).x };
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x);

            std::memcpy(name_buffer, row_name.raw(), sizeof(name_buffer) - 1);
            ImGui::PushID(row);
            ImGui::SetNextItemWidth(label_width - spacing);
            if(ImGui::InputText("##Name", name_buffer, sizeof(name_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue))
                layers.set_name(row, String(str::to_char8(name_buffer), strlen(name_buffer)));
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
