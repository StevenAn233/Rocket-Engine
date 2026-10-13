module;
module EditorSettingPanel;

import EditorLayer;

namespace rke
{
    EditorSettingPanel::EditorSettingPanel(String name, EditorLayer* owner)
        : Panel(std::move(name)), owner_(owner)
        { CORE_ASSERT(owner_, u8"EditorSettingPanel: Owner null!"); }

    EditorSettingPanel::~EditorSettingPanel()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        auto writer{ ConfigWriter::create() };
        if(!writer) return;

        writer->begin_map();

        Project* project{ owner_->context_ };
        writer->write(u8"Last Project Path",
            project ? project->get_rkproj_path().string() : String{});
        auto* scene_edit{ owner_->scene_edit_ };
        writer->write(u8"Scene Edit", scene_edit ? scene_edit->get_name() : String{});

        writer->begin_map(u8"Viewport");
        owner_->post_processor_.serialize_all_to(*(writer.get()));
        writer->end_map();

        writer->begin_map(u8"Style");
        writer->write(u8"Font Scale", font_scale_);
        writer->end_map();

        writer->end_map();
        writer->push_to_file(filepath_);
    }

    void EditorSettingPanel::load_from(Path filepath)
    {
        filepath_ = std::move(filepath);
        if(!filepath_.exists()) {
            CORE_WARN(u8"EditorSettingPanel: File '{}' not found!", filepath_);
            return;
        }
        auto reader{ ConfigReader::create(filepath_) };
        if(!reader || !reader->is_map()) {
            CORE_ERROR(u8"EditorSettingPanel: File format incorrect!");
            return;
        }

        Path proj_dir{ reader->get_at(u8"Last Project Path", String{}) };
        app().load_project(proj_dir);

        String scene_edit_name{ reader->get_at(u8"Scene Edit", String{}) };
        owner_->load_scene_edit(scene_edit_name);

        auto view_data{ reader->get_child(u8"Viewport") };
        if(view_data) owner_->post_processor_
            .deserialize_all_from(*(view_data.get()));
        auto style_data{ reader->get_child(u8"Style") };
        if(style_data) {
            font_scale_ = style_data->get_at(u8"Font Scale", font_scale_);
            ImGui::GetIO().FontGlobalScale = font_scale_;
        }
    }

    void EditorSettingPanel::on_imgui_render()
    {
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        if(!ImGui::BeginTabBar("##editor_settings_tabs")) goto end;
        if(ImGui::BeginTabItem("Viewport"))
        {
            layout::tree_node_branch<u8"Camera">([&]()
            {
                EditorCamera& cam{ owner_->editor_cam_ };
                bool modified{ false };
                ImGui::BeginDisabled(owner_->testing());
                modified |= layout::drag_float3_control<u8"Focus">
                    (cam.focus_, 0.1f, glm::vec3(0.0f));
                modified |= layout::drag_float_control<u8"Distance">
                    (cam.distance_, 0.5f, 1.0f, glm::vec2(1.0f, 100.0f));
                modified |= layout::drag_float_control<u8"Pitch">(cam.pitch_, 0.1f, 0.0f);
                modified |= layout::drag_float_control<u8"Yaw"  >(cam.yaw_  , 0.1f, 0.0f);
                ImGui::EndDisabled();
                if(modified) cam.update_view();
            });

            layout::tree_node_branch<u8"Gizmo">([&]()
            {
                ImGui::BeginDisabled(owner_->testing());
                if(ImGui::RadioButton("Translate", gizmo_mode_ == gizmo::Mode::Translate))
                    gizmo_mode_ = gizmo::Mode::Translate;
                ImGui::SameLine();
                if(ImGui::RadioButton("Rotate", gizmo_mode_ == gizmo::Mode::Rotate))
                    gizmo_mode_ = gizmo::Mode::Rotate;
                ImGui::SameLine();
                if(ImGui::RadioButton("Scale", gizmo_mode_ == gizmo::Mode::Scale))
                    gizmo_mode_ = gizmo::Mode::Scale;
                ImGui::EndDisabled();
            });
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Style"))
        {
            layout::tree_node_branch<u8"Font">([&]()
            {
                if(layout::drag_float_control<u8"Scale">
                    (font_scale_, 0.01f, 1.0f, glm::vec2(0.5f, 2.0f), u8"%.2f")
                ) ImGui::GetIO().FontGlobalScale = font_scale_;
            });
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Post-Process"))
        {
            owner_->post_processor_.on_imgui_render();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    end:
        ImGui::End();
        ImGui::PopID();
    }
}
