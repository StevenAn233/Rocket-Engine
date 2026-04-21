module;

#include <ImGuizmo.h>

module Gizmo;

namespace {
    using namespace rke;

    static ImGuizmo::OPERATION to_imguizmo(Gizmo::Mode mode)
    {
        switch(mode)
        {
        case Gizmo::Mode::Translate: return ImGuizmo::TRANSLATE;
        case Gizmo::Mode::Rotate:    return ImGuizmo::ROTATE;
        case Gizmo::Mode::Scale:     return ImGuizmo::SCALE;
        }
        return ImGuizmo::TRANSLATE;
    }
}

namespace rke
{
    void Gizmo::on_render(Entity selected_entity, Mode mode,
                          const EditorCamera& cam, bool mouse_blocked)
    {
        bool snapping{ Input::is_key_pressed(Key::LeftShift) };

        ImGuizmo::OPERATION gizmo_mode{ to_imguizmo(mode) };

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        // Must be between ImGui::Begin and ImGui::End
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
            static_cast<float>(ImGui::GetWindowWidth ()),
            static_cast<float>(ImGui::GetWindowHeight()));

        if(selected_entity.valid() && !selected_entity.get<TransformComponent>().locked &&
         !(selected_entity.has<CameraComponent>() && (gizmo_mode == ImGuizmo::OPERATION::SCALE)))
        {
            const glm::mat4& cam_proj{ cam.get_proj() };
            const glm::mat4& cam_view{ cam.get_view() };

            auto& tc{ selected_entity.get_mut<TransformComponent>() };
            glm::mat4 transform{ tc.get_transform() };

            float snap_value{ 0.5f };
            if(gizmo_mode == ImGuizmo::OPERATION::ROTATE)
                snap_value = 45.0f;

            float snap_values[3]{ snap_value, snap_value, snap_value };
            ImGuizmo::Manipulate (
                glm::value_ptr(cam_view), glm::value_ptr(cam_proj),
                gizmo_mode, ImGuizmo::WORLD, glm::value_ptr(transform),
                nullptr, snapping ? &snap_values[0] : nullptr);

            if(ImGuizmo::IsUsing() && !mouse_blocked)
            {
                glm::vec3 position{}, rotation{}, scale{};
                ImGuizmo::DecomposeMatrixToComponents
                (
                    glm::value_ptr(transform),
                    glm::value_ptr(position ),
                    glm::value_ptr(rotation ),
                    glm::value_ptr(scale	)
                );

                switch(gizmo_mode)
                {
                case ImGuizmo::OPERATION::TRANSLATE:
                    tc.position = position;
                    break;
                case ImGuizmo::OPERATION::ROTATE:
                {
                    glm::vec3 delta_rotation{ rotation - tc.rotation };
                    tc.rotation += delta_rotation;
                } break;
                case ImGuizmo::OPERATION::SCALE:
                    if(selected_entity.has<CameraComponent>()) {}
                    else if(selected_entity.has<SpriteComponent>())
                    {
                        scale.z = 0.0f;
                        tc.size = scale;
                    } else tc.size = scale;
                    break;
                }
            }
        }
    }

    bool Gizmo::is_over () { return ImGuizmo::IsOver (); }
    bool Gizmo::is_using() { return ImGuizmo::IsUsing(); }
}
