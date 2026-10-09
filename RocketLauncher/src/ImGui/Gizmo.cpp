module;

#include <ImGuizmo.h>

module Gizmo;

namespace {
    using namespace rke;

    static ImGuizmo::OPERATION to_imguizmo(gizmo::Mode mode)
    {
        switch(mode)
        {
        case gizmo::Mode::Translate: return ImGuizmo::TRANSLATE;
        case gizmo::Mode::Rotate:    return ImGuizmo::ROTATE;
        case gizmo::Mode::Scale:     return ImGuizmo::SCALE;
        }
        return ImGuizmo::TRANSLATE;
    }
}

namespace rke::gizmo
{
    void on_render(Scene& scene, Mode mode, const EditorCamera& cam, bool mouse_blocked)
    {
        bool snapping{ app().input().is_key_pressed(Key::LeftShift) };

        ImGuizmo::OPERATION gizmo_mode{ to_imguizmo(mode) };

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        // Must be between ImGui::Begin and ImGui::End
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
            static_cast<float>(ImGui::GetWindowWidth ()),
            static_cast<float>(ImGui::GetWindowHeight()));

        Entity selected_entity{ scene.get_selected_entity() };
        if(!selected_entity.is_valid()) return;
        
        const glm::mat4& cam_proj{ cam.get_proj() };
        const glm::mat4& cam_view{ cam.get_view() };

        const auto& tc{ selected_entity.get<TransformComponent>() };
        glm::mat4 transform {
            selected_entity.get_world_transform().matrix
            * glm::translate(glm::mat4(1.0f), tc.anchor) // ignore anchor
        }; 

        float snap_value{ 0.5f };
        if(gizmo_mode == ImGuizmo::OPERATION::ROTATE) snap_value = 45.0f;

        float snap_values[3]{ snap_value, snap_value, snap_value };
        scene.mark_modified_if(ImGuizmo::Manipulate
        (
            glm::value_ptr(cam_view),
            glm::value_ptr(cam_proj),
            gizmo_mode, ImGuizmo::LOCAL,
            glm::value_ptr(transform), nullptr,
            snapping ? &snap_values[0] : nullptr
        ));

        if(ImGuizmo::IsUsing() && !mouse_blocked)
            selected_entity.set_world_transform(transform
                * glm::translate(glm::mat4(1.0f), -tc.anchor));
    }

    bool is_over () { return ImGuizmo::IsOver (); }
    bool is_using() { return ImGuizmo::IsUsing(); }
}
