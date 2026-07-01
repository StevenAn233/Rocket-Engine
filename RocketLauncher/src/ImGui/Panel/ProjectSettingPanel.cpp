module;

#include <imgui_internal.h>

module ProjectSettingPanel;

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
    void ProjectSettingPanel::on_imgui_render()
    {
        Project* project{ Project::get_active_project() };
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        if(!project) { ImGui::Text("No Active Project"); goto end; }

        if(ImGui::BeginTabBar("##project_settings_tabs"))
        {
            if(ImGui::BeginTabItem("Config"))
            {
                layout::tree_node_branch(u8"Name", [this]()
                {
                    const String& name{ Project::get_active_project()->get_name() };
                    char buffer[256]{};
                    strncpy(buffer, name.raw(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    if(ImGui::InputText("##tag", buffer, sizeof(buffer)))
                        Project::get_active_project()->set_name(String(str::to_char8(buffer)));
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
                layout::tree_node_branch(u8"Anti-Aliasing", [&]()
                {
                    constexpr const char* items[]
                        { "Off", "2x MSAA", "4x MSAA", "8x MSAA", "16x MSAA", "FXAA" };
                    int* aa_opt{ &Project::get_active_project()->get_config_mut().anti_aliasing_opt };
                    if(ImGui::Combo("##msaa", aa_opt, items, static_cast<int>(std::size(items))))
                        apply_aa_setting(*aa_opt);
                });
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    end:
        ImGui::End();
        ImGui::PopID();
    }

    void ProjectSettingPanel::apply_aa_setting(int aa_opt)
    {
        CORE_ASSERT(on_samples_setting_,
            u8"ProjectSettingPanel: Func on_samples_settings() unset!");
        CORE_ASSERT(fxaa_, u8"ProjectSettingPanel: Fxaa handle not set!");

        bool fxaa_enabled{ false };
        if(aa_opt <= 4) {
            fxaa_enabled = false;
            on_samples_setting_(1 << aa_opt);
        } else if(aa_opt == 5) {
            fxaa_enabled = true;
            on_samples_setting_(1);
        }
        fxaa_->set_enabled(fxaa_enabled && viewport_valid_for_fxaa_);
    }

    void ProjectSettingPanel::refresh_aa_setting()
    {
        auto project{ Project::get_active_project() };
        CORE_ASSERT(project, u8"ProjectSettingPanel: No active project!");
        apply_aa_setting(project->get_config().anti_aliasing_opt);
    }

    void ProjectSettingPanel::on_viewport_resized(uint32 w, uint32 h)
    {
        CORE_ASSERT(fxaa_, u8"ProjectSettingPanel: Fxaa handle not set!");
        if(w == 0u || h == 0u) {
            viewport_valid_for_fxaa_ = false;
            fxaa_->set_enabled(false);
        } else {
            viewport_valid_for_fxaa_ = true;
            if(!Project::get_active_project()) return;
            int aa_opt{ Project::get_active_project()->get_config_mut().anti_aliasing_opt };
            fxaa_->set_enabled(aa_opt == 5);
            fxaa_->set_uniform({ glm::vec2(1.0f / w, 1.0f / h) });
        }
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

        for(uint8 row{}; row < count; row++)
        {
            const String& row_name{ layers.get_name(row) };

            float text_w{ ImGui::CalcTextSize(row_name.raw()).x };
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x);

            char buffer[64]{};
            strncpy(buffer, row_name.raw(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
            ImGui::PushID(row);
            ImGui::SetNextItemWidth(label_width - spacing);
            if(ImGui::InputText("##Name", buffer, sizeof(buffer)))
                layers.set_name(row, String(str::to_char8(buffer), strlen(buffer)));
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
                     ImGui::SetTooltip("%s <-> %s", row_name.raw(), layers.get_name(col).raw());

                ImGui::PopID();
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x +
                                     label_width + (col + 1) * (cell_size + spacing));
            }
            ImGui::NewLine();
        }

        ImVec2 cursor_screen_pos{ ImGui::GetCursorScreenPos() };
        float start_x{ cursor_screen_pos.x + label_width };
        float start_y{ cursor_screen_pos.y };
        for(uint8 i{}; i < count; i++)
        {
            const String& text{ layers.get_name(i) };
            float x_pos = start_x + (i * (cell_size + spacing))
                        + ImGui::CalcTextSize(text.raw()).y;
                       // ^ Will be rotated, so use y(height) here
            float y_pos = start_y;

            add_text_vertical(draw_list, text.raw(), ImVec2(x_pos, y_pos), text_col);
        }

        ImGui::PopStyleVar(2);
    }
}
