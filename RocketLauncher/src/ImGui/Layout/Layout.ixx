module;
export module Layout;

import rke;

// assume that all StringViews here are string literals(or at least alive)
export namespace rke::layout
{
    bool drag_float_control (
        StringView label, float& value, float pan_speed = 0.1f,
        float reset_value = 0.0f,
        std::optional<glm::vec2> range = glm::vec2(0.0f, 0.0f),
        StringView format = u8"%.2f");

    bool drag_float2_control (
        StringView label, glm::vec2& value, float pan_speed = 0.1f,
        glm::vec2 reset_value = glm::vec2(0.0f),
        std::optional<glm::vec2> x_range = glm::vec2(0.0f, 0.0f),
        std::optional<glm::vec2> y_range = glm::vec2(0.0f, 0.0f),
        StringView format = u8"%.2f");

    bool drag_float3_control (
        StringView label, glm::vec3& values, float pan_speed = 0.1f,
        glm::vec3 reset_value = glm::vec3(0.0f),
        std::optional<glm::vec2> x_range = glm::vec2(0.0f, 0.0f),
        std::optional<glm::vec2> y_range = glm::vec2(0.0f, 0.0f),
        std::optional<glm::vec2> z_range = glm::vec2(0.0f, 0.0f),
        StringView format = u8"%.2f");

    template<typename Callback>
    requires std::invocable<Callback>
    void two_columns_table(StringView text, Callback&& callback)
    {
        constexpr ImGuiTableFlags table_flags
        {   ImGuiTableFlags_Resizable
          | ImGuiTableFlags_SizingStretchProp
          | ImGuiTableFlags_BordersInnerV
        };

        ImGui::PushID(text.raw_unsafe(), text.raw_unsafe() + text.size());
        if(ImGui::BeginTable("", 2, table_flags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(text.raw_unsafe(), text.raw_unsafe() + text.size());
            ImGui::TableNextColumn();

            std::invoke(std::forward<Callback>(callback));
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    template<typename Callback>
    requires std::invocable<Callback>
    void tree_node_branch(StringView name, Callback&& callback,
                          ImGuiTreeNodeFlags extra_flags = 0,
                          void* id = nullptr)
    {
        constexpr ImGuiTreeNodeFlags branch_flags
        {   ImGuiTreeNodeFlags_DefaultOpen
          | ImGuiTreeNodeFlags_AllowOverlap
          | ImGuiTreeNodeFlags_SpanAvailWidth
          | ImGuiTreeNodeFlags_FramePadding
          | ImGuiTreeNodeFlags_DrawLinesToNodes
        };

        if(ImGui::TreeNodeEx(id ? id : std::bit_cast<void*>(name.data()),
           branch_flags | extra_flags, "%.*s",
           static_cast<int>(name.size()), name.raw_unsafe()))
        {
            std::invoke(std::forward<Callback>(callback));
            ImGui::TreePop();
        }
    }

    template<typename Callback>
    requires std::invocable<Callback>
    void tree_node_leaf(StringView name, Callback&& callback)
    {
        constexpr ImGuiTreeNodeFlags leaf_flags
        {   ImGuiTreeNodeFlags_Leaf
          | ImGuiTreeNodeFlags_AllowOverlap
          | ImGuiTreeNodeFlags_SpanAvailWidth
          | ImGuiTreeNodeFlags_FramePadding
        };

        if(ImGui::TreeNodeEx(std::bit_cast<void*>(name.data()),
            leaf_flags, "%.*s", static_cast<int>(name.size()), name.raw_unsafe()))
        {
            std::invoke(std::forward<Callback>(callback));
            ImGui::TreePop();
        }
    }
}
