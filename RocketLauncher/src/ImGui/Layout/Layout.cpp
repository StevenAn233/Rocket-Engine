module;

#include <imgui_internal.h>

module Layout;

namespace {
    static const ImVec4 s_grey   (0.7f, 0.7f, 0.70f, 1.0f);
    static const ImVec4 s_greyer (0.9f, 0.9f, 0.90f, 1.0f);

    static const ImVec4 s_red	 (0.8f, 0.1f, 0.15f, 1.0f);
    static const ImVec4 s_reder	 (0.9f, 0.2f, 0.20f, 1.0f);

    static const ImVec4 s_green	 (0.2f, 0.7f, 0.20f, 1.0f);
    static const ImVec4 s_greener(0.3f, 0.8f, 0.30f, 1.0f);

    static const ImVec4 s_blue	 (0.1f, 0.25f, 0.8f, 1.0f);
    static const ImVec4 s_bluer	 (0.2f, 0.35f, 0.9f, 1.0f);
}

namespace rke::layout
{
    bool drag_float_control (
        StringView label, float& value, float pan_speed,
        float reset_value,
        std::optional<glm::vec2> range,
        StringView format)
    {
        bool data_changed{ false };
        ImGui::PushID(label.raw_unsafe());
        two_columns_table(label, [&]()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, 0.0f });

            float  line_height{ ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f };
            ImVec2 button_size{ line_height * 0.5f, line_height };

            float available_width{ ImGui::GetContentRegionAvail().x };
            float item_width {
                available_width - button_size.x -
                GImGui->Style.ItemSpacing.x * 2.0f
            };

            ImGui::PushStyleColor(ImGuiCol_Button,		  s_grey  );
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_greyer);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_grey  );
            if(ImGui::Button("##reset", button_size))
            {
                if(range.has_value())
                {
                    value = reset_value;
                    data_changed = true;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(item_width);
            if(!range) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##value", &value, 0.0f, 0.0f, 0.0f, format.raw_unsafe());
                ImGui::EndDisabled();
            } else {
                if(ImGui::DragFloat("##value", &value,
                    pan_speed, range->x, range->y, format.raw_unsafe()))
                    data_changed = true;
            }
            ImGui::PopStyleVar();
        });
        ImGui::PopID();
        return data_changed;
    }

    bool drag_float2_control (
        StringView label, glm::vec2& value, float pan_speed,
        glm::vec2 reset_value,
        std::optional<glm::vec2> x_range,
        std::optional<glm::vec2> y_range,
        StringView format)
    {
        bool data_changed{ false };
        ImGui::PushID(label.raw_unsafe());
        two_columns_table(label, [&]()
        {
            float  per_item_spacing_x{ GImGui->Style.ItemSpacing.x };
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, 0.0f });

            float  line_height{ ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f };
            ImVec2 button_size{ line_height * 0.5f, line_height };

            float available_width{ ImGui::GetContentRegionAvail().x };
            float item_width { available_width / 2.0f
                - button_size.x
                - GImGui->Style.ItemSpacing.x };

            ImGui::PushStyleColor(ImGuiCol_Button,		  s_grey  );
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_greyer);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_grey  );
            if(ImGui::Button("##reset_x", button_size))
            {
                if(x_range.has_value())
                {
                    value.x = reset_value.x;
                    data_changed = true;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(item_width);
            if(!x_range) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##value_x", &value.x, 0.0f, 0.0f, 0.0f, format.raw_unsafe());
                ImGui::EndDisabled();
            } else {
                if(ImGui::DragFloat("##value_x", &value.x,
                    pan_speed, x_range->x, x_range->y, format.raw_unsafe()))
                    data_changed = true;
            }
            ImGui::SameLine(0.0f, per_item_spacing_x);


            ImGui::PushStyleColor(ImGuiCol_Button,		  s_grey  );
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_greyer);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_grey  );
            if(ImGui::Button("##reset_y", button_size))
            {
                if(y_range.has_value())
                {
                    value.y = reset_value.y;
                    data_changed = true;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(item_width);
            if(!y_range) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##value_y", &value.y, 0.0f, 0.0f, 0.0f, format.raw_unsafe());
                ImGui::EndDisabled();
            } else {
                if(ImGui::DragFloat("##value_y", &value.y, pan_speed,
                    y_range->x, y_range->y, format.raw_unsafe()))
                    data_changed = true;
            }

            ImGui::PopStyleVar();
        });
        ImGui::PopID();
        return data_changed;
    }

    bool drag_float3_control (
        StringView label, glm::vec3& values,
        float pan_speed, glm::vec3 reset_value,
        std::optional<glm::vec2> x_range,
        std::optional<glm::vec2> y_range,
        std::optional<glm::vec2> z_range,
        StringView format)
    {
        bool data_changed{ false };
        ImGui::PushID(label.raw_unsafe());
        two_columns_table(label, [&]()
        {
            float  per_item_spacing_x{ GImGui->Style.ItemSpacing.x };
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, 0.0f });

            float  line_height{ ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f };
            ImVec2 button_size{ line_height - 3.0f, line_height };

            float available_width{ ImGui::GetContentRegionAvail().x };
            float item_width { (available_width - 10.0f) / 3.0f
                - button_size.x
                - GImGui->Style.ItemSpacing.x * 2.0f
            };

        // X: Red Button Style
            ImGui::PushStyleColor(ImGuiCol_Button,        s_red  );
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_reder);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_red  );
            if(ImGui::Button("X", button_size))
            {
                if(x_range.has_value())
                {
                    values.x = reset_value.x;
                    data_changed = true;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            if(!x_range) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##x", &values.x, 0.0f, 0.0f, 0.0f, format.raw_unsafe());
                ImGui::EndDisabled();
            } else {
                if(ImGui::DragFloat("##x", &values.x, pan_speed,
                    x_range->x, x_range->y, format.raw_unsafe()))
                    data_changed = true;
            }
            ImGui::SameLine(0.0f, per_item_spacing_x);

        // Y: Green Button Style
            ImGui::PushStyleColor(ImGuiCol_Button,		  s_green  );
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_greener);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_green  );
            if(ImGui::Button("Y", button_size))
            {
                if(y_range.has_value())
                {
                    values.y = reset_value.y;
                    data_changed = true;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            if(!y_range) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##y", &values.y, 0.0f, 0.0f, 0.0f, format.raw_unsafe());
                ImGui::EndDisabled();
            } else {
                if(ImGui::DragFloat("##y", &values.y, pan_speed,
                    y_range->x, y_range->y, format.raw_unsafe()))
                    data_changed = true;
            }
            ImGui::SameLine(0.0f, per_item_spacing_x);

        // Z: Blue Button Style
            ImGui::PushStyleColor(ImGuiCol_Button,		  s_blue );
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_bluer);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_blue );
            if(ImGui::Button("Z", button_size))
            {
                if(z_range.has_value())
                {
                    values.z = reset_value.z;
                    data_changed = true;
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            if(!z_range) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##z", &values.z, 0.0f, 0.0f, 0.0f, format.raw_unsafe());
                ImGui::EndDisabled();
            } else {
                if(ImGui::DragFloat("##z", &values.z, pan_speed,
                    z_range->x, z_range->y, format.raw_unsafe()))
                    data_changed = true;
            }

            ImGui::PopStyleVar();
        });
        ImGui::PopID();
        return data_changed;
    }
}
