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

        Project* project{ app().get_project() };
        writer->write(u8"Last Project Path",
            project ? project->get_rkproj_path().string() : String{});
        writer->write(u8"Scene Edit Path", owner_->scene_edit_path_.string());

        writer->begin_map(u8"Viewport");
        selected_outline()->serialize_to(*(writer.get()));
        hovering_outline()->serialize_to(*(writer.get()));
        writer->write(u8"Selected Enabled", selected_enabled_editor());
        writer->write(u8"Hovering Enabled", hovering_enabled_editor());
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

        Project* project{ app().get_project() };
        Path scene_edit_path{ reader->get_at(u8"Scene Edit Path", String{}) };
        if(!project || scene_edit_path.parent_path() != project->get_scenes_dir())
            CORE_ERROR(u8"EditorSettingPanel: Scene doesn't belong to current project!");
        else owner_->load_scene_edit_from(scene_edit_path);

        auto view_data{ reader->get_child(u8"Viewport") };
        if(view_data) {
            selected_outline()->deserialize_from(*(view_data.get()));
            hovering_outline()->deserialize_from(*(view_data.get()));
            selected_enabled_editor_ = view_data->get_at(u8"Selected Enabled", true);
            hovering_enabled_editor_ = view_data->get_at(u8"Hovering Enabled", true);
        }
        auto style_data{ reader->get_child(u8"Style") };
        if(style_data) {
            font_scale_ = style_data->get_at(u8"Font Scale", font_scale_);
            ImGui::GetIO().FontGlobalScale = font_scale_;
        }
    }

    void EditorSettingPanel::set_outline_samples(uint32 samples)
    {
        selected_outline()->set_samples(samples);
        hovering_outline()->set_samples(samples);
    }

    void EditorSettingPanel::on_imgui_render()
    {
        CORE_ASSERT(selected_outline(), u8"EditorSettingPanel: Selected outline handle not set!");
        CORE_ASSERT(hovering_outline(), u8"EditorSettingPanel: Hovering outline handle not set!");
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        if(!ImGui::BeginTabBar("##editor_settings_tabs")) goto end;
        if(ImGui::BeginTabItem("Viewport"))
        {
            layout::tree_node_branch<u8"Camera">([&]()
            {
                EditorCamera& cam{ owner_->editor_cam_ };
                bool modified{ false };
                modified |= layout::drag_float3_control<u8"Focus">
                    (cam.focus_, 0.1f, glm::vec3(0.0f));
                modified |= layout::drag_float_control<u8"Distance">
                    (cam.distance_, 0.5f, 1.0f, glm::vec2(1.0f, 100.0f));
                modified |= layout::drag_float_control<u8"Pitch">(cam.pitch_, 0.1f, 0.0f);
                modified |= layout::drag_float_control<u8"Yaw"  >(cam.yaw_  , 0.1f, 0.0f);
                if(modified) cam.update_view();
            });

            layout::tree_node_branch<u8"Gizmo">([&]()
            {
                if(ImGui::RadioButton("Translate", gizmo_mode_ == Gizmo::Mode::Translate))
                    gizmo_mode_ = Gizmo::Mode::Translate;
                ImGui::SameLine();
                if(ImGui::RadioButton("Rotate", gizmo_mode_ == Gizmo::Mode::Rotate))
                    gizmo_mode_ = Gizmo::Mode::Rotate;
                ImGui::SameLine();
                if(ImGui::RadioButton("Scale", gizmo_mode_ == Gizmo::Mode::Scale))
                    gizmo_mode_ = Gizmo::Mode::Scale;
            });
            
            layout::tree_node_branch<u8"Selected Outline">([&]()
            {
                float outline_thickness{ selected_outline()->get_thickness() };
                if(layout::drag_float_control<u8"Thickness">
                    (outline_thickness, 0.01f, 1.0f, glm::vec2(0.0f, 2.0f)))
                    { selected_outline()->set_thickness(outline_thickness); }
                ImGui::Checkbox("Enabled", &selected_enabled_editor_);
            });

            layout::tree_node_branch<u8"Hovering Outline">([&]()
            {
                float outline_thickness{ hovering_outline()->get_thickness() };
                if(layout::drag_float_control<u8"Thickness">
                    (outline_thickness, 0.01f, 1.0f, glm::vec2(0.0f, 2.0f)))
                    { hovering_outline()->set_thickness(outline_thickness); }
                ImGui::Checkbox("Enabled", &hovering_enabled_editor_);
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
        ImGui::EndTabBar();
    end:
        ImGui::End();
        ImGui::PopID();
    }

    OutlineEffect* EditorSettingPanel::hovering_outline() { return owner_->hovering_outline_; }
    OutlineEffect* EditorSettingPanel::selected_outline() { return owner_->selected_outline_; }
}
