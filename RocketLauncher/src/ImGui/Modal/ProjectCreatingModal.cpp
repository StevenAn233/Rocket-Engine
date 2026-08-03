module;
module ProjectCreatingModal;

namespace rke
{
    ProjectCreatingModal::ProjectCreatingModal(String title)
        : Modal(std::move(title))
    {
        name_buffer_ = create_scope<std::array<char, 256>>();
        path_buffer_ = create_scope<std::array<char, 512>>();
        std::memcpy(name_buffer_->data(), "New Project", 12);
        copy_to_buffer(file::root_dir().string());
    }

    void ProjectCreatingModal::on_imgui_render()
    {
        in_use_ = ImGui::BeginPopupModal (
            get_title().raw(), nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        );
        if(in_use()) {
            ImGui::Text("Project Name:");
            ImGui::InputText("##ProjectName", name_buffer_->data(), name_buffer_->size());

            ImGui::Text("Location:");
            ImGui::InputText("##ProjectLoc", path_buffer_->data(), path_buffer_->size());
            ImGui::SameLine();
            if(ImGui::Button("..."))
            {
                String selected_folder{ FileDialogs::select_folder
                    (app().get_windows_lib().get_main().get_context()) };
                copy_to_buffer(selected_folder);
            }

            ImGui::Separator();

            if(ImGui::Button("Create", ImVec2(120, 0)))
            {
                if(name_buffer_->size() > 0)
                {
                    String name{ str::to_char8(name_buffer_->data()) };
                    Path project_dir{ Path(path_buffer_->data()) / name };
                    if(project_dir.exists()) {
                        CORE_ERROR(u8"ProjectCreatingModal: Project dir already exists!");
                    } else {
                        Path rkproj_path{ project_dir / (name + u8".rkproj")};
                        if(Project::create_files(rkproj_path))
                        {
                            if(on_project_created_) on_project_created_(rkproj_path);
                            ImGui::CloseCurrentPopup();
                        }
                        else CORE_ERROR(u8"ProjectCreatingModal: Failed to create_ref rkproj!");
                    }
                }
                else CORE_WARN(u8"ProjectCreatingModal: Project name cannot be empty!");
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();

            in_use_ = false;
            ImGui::EndPopup();
        }
    }

    void ProjectCreatingModal::copy_to_buffer(const String& path)
    {
        if(path.length() > path_buffer_->size() - 1)
            { CORE_ERROR(u8"ProjectCreatingModal: Path too long!"); return; }
        std::memcpy(path_buffer_->data(), path.raw(), path.length());
        (*path_buffer_)[path.length()] = '\0';
    }
}
