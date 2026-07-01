module;
module EditorSettingPanel;

import Layout;

namespace rke
{
    EditorSettingPanel::~EditorSettingPanel()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        auto writer{ ConfigWriter::create() };
        if(!writer) {
            CORE_ERROR(u8"Dockspace: Failed to create config writer!");
            return;
        }
        writer->begin_map();
        writer->write(u8"Last Project Path", last_proj_path_.string());

        writer->begin_map(u8"Viewport");
        selected_->serialize_to(writer.get());
        hovering_->serialize_to(writer.get());
        writer->write(u8"Selected Enabled", selected_enabled_editor());
        writer->write(u8"Hovering Enabled", hovering_enabled_editor());
        writer->end_map();

        writer->begin_map(u8"Style");
        writer->write(u8"Font Scale", font_scale_);
        writer->end_map();

        writer->end_map();
        writer->push_to_file(filepath_);
    }

    void EditorSettingPanel::on_imgui_render()
    {
        CORE_ASSERT(selected_, u8"EditorSettingPanel: Selected outline handle not set!");
        CORE_ASSERT(hovering_, u8"EditorSettingPanel: Hovering outline handle not set!");
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        if(ImGui::BeginTabBar("##editor_settings_tabs"))
        {
            if(ImGui::BeginTabItem("Viewport"))
            {
                layout::tree_node_branch(u8"Gizmo", [&]()
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
                
                layout::tree_node_branch(u8"Selected Outline", [&]()
                {
                    selected_->on_imgui_render();
                    ImGui::Checkbox("Enabled", &selected_enabled_editor_);
                });

                layout::tree_node_branch(u8"Hovering Outline", [&]()
                {
                    hovering_->on_imgui_render();
                    ImGui::Checkbox("Enabled", &hovering_enabled_editor_);
                });

                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Style"))
            {
                layout::tree_node_branch(u8"Font", [&]()
                {
                    if(layout::drag_float_control
                    (u8"Scale", font_scale_, 0.01f, 1.0f, glm::vec2(0.5f, 2.0f), u8"%.2f"))
                        ImGui::GetIO().FontGlobalScale = font_scale_;
                });
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
        ImGui::PopID();
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

        last_proj_path_ = reader->get_at(u8"Last Project Path", String{});
        if(!last_proj_path_.empty() && !last_proj_path_.exists())
        {
            CORE_ERROR(u8"EditorSettingPanel: Last project path not found!");
            last_proj_path_.clear();
        }
        auto view_data{ reader->get_child(u8"Viewport") };
        if(view_data) {
            selected_->deserialize_from(view_data.get());
            hovering_->deserialize_from(view_data.get());
            selected_->set_enabled(view_data->get_at(u8"Selected Enabled", true));
            hovering_->set_enabled(view_data->get_at(u8"Hovering Enabled", true));
        }
        
        auto style_data{ reader->get_child(u8"Style") };
        if(style_data) {
            font_scale_ = style_data->get_at(u8"Font Scale", font_scale_);
            ImGui::GetIO().FontGlobalScale = font_scale_;
        }
    }

    void EditorSettingPanel::set_outline_samples(uint32 samples)
    {
        selected_->set_samples(samples);
        hovering_->set_samples(samples);
    }

    void EditorSettingPanel::set_last_proj_path(Path path)
        { last_proj_path_ = std::move(path); }

    void EditorSettingPanel::on_viewport_resized(uint32 w, uint32 h)
    {
        selected_->on_viewport_resized(w, h);
        hovering_->on_viewport_resized(w, h);
    }
}
