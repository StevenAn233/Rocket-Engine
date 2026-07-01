module;
module ProjectCreatingModal;

namespace rke
{
    ProjectCreatingModal::ProjectCreatingModal(String title) : Modal(std::move(title)) {}

    void ProjectCreatingModal::popup() { ImGui::OpenPopup(get_title().raw()); }

    void ProjectCreatingModal::on_render(const Window* window)
    {
        if(ImGui::BeginPopupModal(get_title().raw(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            in_use_ = true;
            static char name_buffer[256]{ "Project" }; // saved during whole exe life-time

        #ifdef RKE_PLATFORM_WINDOWS
            static char loc_buffer[256]{ "C:\\"	}; // can memorize during programme life
        #else
            static_assert(false, u8"Project: Other OS has not been supported yet!");
        #endif

            ImGui::Text("Project Name:");
            ImGui::InputText("##ProjectName", name_buffer, sizeof(name_buffer));

            ImGui::Text("Location:");
            ImGui::InputText("##ProjectLoc", loc_buffer, sizeof(loc_buffer));
            ImGui::SameLine();
            if(ImGui::Button("..."))
            {
                auto selected_folder{ FileDialogs::select_folder(window) };
                if(selected_folder) strncpy(loc_buffer,
                    (*selected_folder).raw(), sizeof(loc_buffer) - 1);
                loc_buffer[sizeof(loc_buffer) - 1] = '\0';
            }

            ImGui::Separator();

            if(ImGui::Button("Create", ImVec2(120, 0)))
            {
                if(strlen(name_buffer) > 0)
                {
                    String name{ str::to_char8(name_buffer) };
                    Path project_dir { Path(loc_buffer) / name };
                    Path rkproj_path{ project_dir / (name + u8".rkproj") };

                    if(Project::create(rkproj_path))
                    {
                        if(on_project_created_)
                            on_project_created_(rkproj_path);
                    }
                    else CORE_ERROR(u8"EditorLayer: Failed to create_ref rkproj!");

                    ImGui::CloseCurrentPopup();
                }
                else CORE_WARN(u8"EditorLayer: Project name cannot be empty!");
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
        else in_use_ = false;
    }

    void ProjectCreatingModal::set_project_created_callback(ProjectCreatedCallback callback)
        { on_project_created_ = std::move(callback); }
}
