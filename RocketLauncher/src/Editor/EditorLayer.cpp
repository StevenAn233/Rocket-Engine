module;
module EditorLayer;

import Gizmo;
import OutlineEffect;
import FXAAEffect;

namespace rke
{
    EditorLayer::EditorLayer(String window_title, String name)
        : Layer(std::move(window_title), std::move(name)) {}

    void EditorLayer::on_attach()
    {
        Layer::on_attach();
        const auto& app{ Application::get() };

        Scene::set_on_entity_selected([this](Entity entity)
            { editor_setting_panel_.get_selected()->set_target(entity); });

        SceneSerializer::set_serialize_hook
        ([this](SceneSerializer* self, ConfigWriter* writer)
        {
            if(!self->is_to_serialize(current_scene_.get())) return;
            editor_cam_.serialize_to(writer);
            
            Entity cam_demo_target{ cam_renderer_.get_cam_demo_target() };
            if(cam_demo_target.valid()) writer->write
                (u8"Cam Demo Target", cam_demo_target.get_uuid().value());
            else if(!cam_demo_target.empty())
                CORE_ERROR(u8"EditorLayer: Cam demo target invalid!");
        });

        SceneSerializer::set_deserialize_hook
        ([this](SceneSerializer*, Scene* scene, const ConfigReader* reader)
        {
            editor_cam_.deserialize_from(reader);
            if(reader->has_key(u8"Cam Demo Target")) {
                UUID id{reader->get_at(u8"Cam Demo Target", 0ui64)};
                cam_renderer_.set_cam_demo_target(scene->get_entity(id));
            }
            else cam_renderer_.set_cam_demo_target({});
        });

    // DockSpace
        dockspace_.load_from(file::find_editor_dir() / u8"settings" / u8"dockspace.yaml");
        dockspace_.set_menubar_callback([this]()
        {
            if(ImGui::BeginMenu("Window")) {
                if(ImGui::MenuItem("Close", "Alt+F4"))
                    Application::get().remove_window(get_owner_title());
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("File")) {
                bool is_play{ scene_state_ == SceneState::Play };
                bool no_project{ !Project::get_active_project() };
                if(ImGui::MenuItem("New Project...", "Ctrl+N",
                    false, !is_play)) new_project();
                if(ImGui::MenuItem("Open Project...", "Ctrl+O",
                    false, !is_play)) open_project(get_owner());
                if(ImGui::MenuItem("Save Project", "Ctrl+S",
                    false, !is_play && !no_project)) save_project();
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Panels")) {
                panel_registry_.render_switches_menubar();
                ImGui::EndMenu();
            }
        });

    // Post-Processing Effects
        auto hovering{ create_scope<OutlineEffect>(u8"Hovering") };
        auto selected{ create_scope<OutlineEffect>(u8"Selected") };
        hovering->set_color(glm::vec4(1.0f, 0.8f, 0.0f, 1.0f));
        selected->set_color(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));

        editor_setting_panel_.set_hovering_handle(hovering.get());
        editor_setting_panel_.set_selected_handle(selected.get());
        editor_setting_panel_.load_from(file::find_editor_dir() / u8"settings" / u8"editor.yaml");

        auto fxaa{ create_scope<FXAAEffect>(u8"Fxaa") };

        project_setting_panel_.set_fxaa_handle(fxaa.get());
        project_setting_panel_.set_on_samples_setting([this](uint32 samples)
        {
            main_renderer_.set_samples(samples);
            editor_setting_panel_.set_outline_samples(samples);
        });

        main_renderer_.set_samples(4);
        main_renderer_.add_effect(std::move(hovering));
        main_renderer_.add_effect(std::move(selected));
        main_renderer_.add_effect(std::move(fxaa));

        cam_renderer_.set_samples(1);

    // SceneHierarchy
        scene_hierarchy_panel_.set_on_entity_node_render([this](Scene* scene)
            { entity_right_click_popup_content(scene); });

    // WindowSetting
        window_setting_panel_.set_context(get_owner());

    // ContentBrowser
        content_browser_panel_.set_folder_icon
            (app.asset_path(Path(u8"icons") / u8"folder.png"));
        content_browser_panel_.set_image_icon
            (app.asset_path(Path(u8"icons") / u8"image.png" ));
        content_browser_panel_.set_file_icon
            (app.asset_path(Path(u8"icons") / u8"file.png"  ));
        content_browser_panel_.load_from
            (file::find_editor_dir() / u8"settings" / u8"content-browser.yaml");

    // Toolbar
        toolbar_.emplace_icon_button(u8"Play",
            Texture2D::create(app.asset_path(Path(u8"icons") / u8"play.png"),
                Texture::FiltFormat::Linear, Texture::WrapFormat::Clamp2Edge, false),
            [this](IconButton*) { on_runtime_start(); },
            [this]() { return current_scene_ && editing(); });

        toolbar_.emplace_icon_button(u8"Stop",
            Texture2D::create(app.asset_path(Path(u8"icons") / u8"stop.png"),
                Texture::FiltFormat::Linear, Texture::WrapFormat::Clamp2Edge, false),
            [this](IconButton*) { on_runtime_stop(); },
            [this]() { return current_scene_ && playing(); });

        toolbar_.emplace_icon_button(u8"Reload Scripts",
            Texture2D::create(app.asset_path(Path(u8"icons") / u8"refresh.png"),
                Texture::FiltFormat::Linear, Texture::WrapFormat::Clamp2Edge, false),
            [this](IconButton*) {
                auto project{ Project::get_active_project() };
                project->scripts_hot_reloading();
                update_current_scene(project->get_active_scene());
            },
            [this]() {
                return Project::get_active_project()
                    && current_scene_ && !current_scene_->in_runtime();
            });

    // Viewports
        main_viewport_.set_in_viewport_callback([this](Viewport* self)
        {
        #ifndef RKE_SHIPPING
            Application::get().get_imgui_layer()
                ->set_main_viewport_hovered(self->is_hovered());
            Application::get().get_imgui_layer()
                ->set_main_viewport_focused(self->is_focused());
        #endif
            if(current_scene_ && self->is_focused() && editing()) {
                Gizmo::on_render(current_scene_->get_selected_entity(),
                                 editor_setting_panel_.get_gizmo_mode(),
                                 editor_cam_, mouse_blocked());
            }

            if((!current_scene_ || editing()) && ImGui::BeginDragDropTarget())
            {
                in_main_viewport_dragging_ = true;
                const auto* payload{ ImGui::AcceptDragDropPayload("CONTENT_BROWSER_SCENE") };
                if(payload) {
                    String scene_path{ reinterpret_cast<const char8*>(payload->Data) };
                    if(Project::get_active_project() && scene_path.ends_with(u8".rkscene"))
                    {
                        Ref<Scene> active_scene
                            { Project::get_active_project()->load_scene(scene_path) };
                        update_current_scene(active_scene); // can be nullptr
                    }
                }
                ImGui::EndDragDropTarget();
            }
            else in_main_viewport_dragging_ = false;

            static bool in_popup{ false };
            if(!current_scene_ || (hovering_id_ == -1 && !in_popup)) return;
            if(ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight))
            {
                if(ImGui::IsWindowAppearing()) {
                    in_popup = true;
                    current_scene_->set_selected_entity(hovering_id_);
                }
                entity_right_click_popup_content(current_scene_.get());
                ImGui::EndPopup();
            }
            else in_popup = false;
        });

    // PanelRegistry
        panel_registry_.set_switches_load_from
            (file::find_editor_dir() / u8"settings" / u8"panel-switches.yaml");
        panel_registry_.register_panel(&application_panel_	  );
        panel_registry_.register_panel(&window_setting_panel_ );
        panel_registry_.register_panel(&scene_hierarchy_panel_);
        panel_registry_.register_panel(&editor_setting_panel_ );
        panel_registry_.register_panel(&content_browser_panel_);
        panel_registry_.register_panel(&project_setting_panel_);
        panel_registry_.register_panel(&main_viewport_, false);
        panel_registry_.register_panel(&cam_viewport_ , true,
            [this]() { return scene_state_ == SceneState::Edit; });
        panel_registry_.register_panel(&toolbar_, false);

    // Modal(s)
        project_creating_modal_.set_project_created_callback
        ([this](const Path& rkproj_path)
            { update_current_scene({}); open_project(rkproj_path); });

    // Load last project(after editor_setting_panel_.load_from(...))
        const Path& last_proj_path{ editor_setting_panel_.get_last_proj_path() };
        if(last_proj_path.exists()) open_project(last_proj_path);
        else if(!last_proj_path.empty())
            CORE_ERROR(u8"EditorLayer: Project '{}' not found!", last_proj_path);
    }

    void EditorLayer::on_detach()
    {
        if(playing()) on_runtime_stop();
        update_current_scene(nullptr);

        if(Project::get_active_project())
            Project::get_active_project()->clear_active_scene();
        Project::release_active();
    }

    void EditorLayer::on_update(float dt)
    {
        RKE_PROFILE_FUNCTION();

        Layer::on_update(dt);

        glm::vec2 vp_size{ main_viewport_.get_viewport_size() };
        glm::vec2 cd_size{ cam_viewport_ .get_viewport_size() };

        if(last_vp_size_.x != vp_size.x || last_vp_size_.y != vp_size.y)
        {
            auto w{ static_cast<uint32>(vp_size.x) };
            auto h{ static_cast<uint32>(vp_size.y) };

            editor_cam_.set_viewport(w, h);
            if(current_scene_) current_scene_->set_viewport(w, h);

            main_renderer_.on_viewport_resized(w, h);
            editor_setting_panel_ .on_viewport_resized(w, h);
            project_setting_panel_.on_viewport_resized(w, h);

            last_vp_size_ = vp_size;
        }
        if(last_cd_size_.x != cd_size.x || last_cd_size_.y != cd_size.y)
        {
            auto w{ static_cast<uint32>(cd_size.x) };
            auto h{ static_cast<uint32>(cd_size.y) };

            cam_renderer_.on_viewport_resized(w, h);

            last_cd_size_ = cd_size;
        }

        if(!current_scene_) return;

        bool hovering_enabled{ !Gizmo::is_using()
            && editor_setting_panel_.hovering_enabled_editor()
            && !in_main_viewport_dragging_ && editing()
            && !project_creating_modal_.in_use() && main_viewport_.is_focused()
        };
        editor_setting_panel_.get_hovering()->set_enabled(hovering_enabled);

        Entity selected{ current_scene_->get_selected_entity() };
        int selected_id{ selected.valid() ? static_cast<int>(selected.get_handle()) : -1 };
        bool selected_enabled{ !Gizmo::is_using()
            && editor_setting_panel_.selected_enabled_editor()
            && !in_main_viewport_dragging_ && editing()
            && !project_creating_modal_.in_use()
            && (selected_id != hovering_id_ || !hovering_enabled)
        };
        editor_setting_panel_.get_selected()->set_enabled(selected_enabled);

        switch(scene_state_)
        {
        case SceneState::Edit:
            editor_cam_.on_update(dt);
            break;
        default: break;
        }

        current_scene_->on_update(dt); // entity deleted here
        cam_renderer_.cam_demo_validation_check();
    }

    void EditorLayer::on_render()
    {
        switch(scene_state_)
        {
        case SceneState::Edit:
            main_output_ = main_renderer_.on_render
                (current_scene_.get(), editor_cam_.get_view_proj(), editor_cam_.get_pos());

            if(current_scene_&& main_viewport_.is_hovered() &&
             !(Gizmo::is_over() && current_scene_->get_selected_entity().valid()))
            {
                glm::vec2 vp_mouse{ main_viewport_.get_viewport_mouse() };
                hovering_id_ = main_renderer_.get_hovering_id(vp_mouse.x, vp_mouse.y);
            }
            else hovering_id_ = -1;

            if(current_scene_) {
                Entity target{ current_scene_->get_entity(hovering_id_, false) };
                editor_setting_panel_.get_hovering()->set_target(target);
            }

            if(current_scene_ && last_cd_size_.x > 0 && last_cd_size_.y > 0)
            {
                // switch to cam demo viewport size
                current_scene_->set_viewport(last_cd_size_.x, last_cd_size_.y);
                cam_output_ = cam_renderer_.cam_demo_render
                    (current_scene_.get(), current_scene_->get_selected_entity());
                current_scene_->set_viewport(last_vp_size_.x, last_vp_size_.y);
            }
            else cam_output_ = nullptr;
            break;
        case SceneState::Play:
            main_output_ = main_renderer_.on_render_runtime(current_scene_.get());
            cam_output_  = nullptr;
            break;
        default: break;
        }

        if(main_output_) main_viewport_.
            set_next_render_target(main_output_->get_renderer_id());
        if(cam_output_ ) cam_viewport_.
            set_next_render_target(cam_output_ ->get_renderer_id());
    }

    void EditorLayer::on_imgui_render()
    {
        RKE_PROFILE_FUNCTION();

        dockspace_.on_imgui_render();

        panel_registry_.on_imgui_render();
        
        if(to_create_new_proj_)
        {
            project_creating_modal_.popup();
            to_create_new_proj_ = false;
        }
        project_creating_modal_.on_render(get_owner());
    }

    void EditorLayer::on_runtime_start()
    {
        CORE_ASSERT(current_scene_, u8"EditorLayer: Current scene empty!");
        if(current_scene_->in_runtime())
        {
            CORE_ERROR(u8"EditorLayer: Scene-state chaotic!");
            return;
        }

        UUID last_selected{ current_scene_->get_selected_entity().get_uuid() };
        origin_current_scene_ = current_scene_;

        update_current_scene(current_scene_->deep_copy());
        if(last_selected) current_scene_->set_selected_entity(last_selected);
        else current_scene_->set_selected_entity(Entity());
        current_scene_->on_runtime_start();

        scene_state_ = SceneState::Play;
    }

    void EditorLayer::on_runtime_stop()
    {
        CORE_ASSERT(current_scene_, u8"EditorLayer: Current scene empty!");
        if(!current_scene_->in_runtime())
        {
            CORE_ERROR(u8"EditorLayer: Scene-state chaotic!");
            return;
        }
        scene_state_ = SceneState::Edit;

        UUID last_selected{ current_scene_->get_selected_entity().get_uuid() };
        current_scene_->on_runtime_stop();

        update_current_scene(origin_current_scene_);
        if(last_selected) current_scene_->set_selected_entity(last_selected);
        else current_scene_->set_selected_entity(Entity());
        origin_current_scene_ = nullptr;
    }

    void EditorLayer::new_project() { to_create_new_proj_ = true; }

    void EditorLayer::open_project(const Path& rkproj_path)
    {
        Project::load_to_active(rkproj_path);
        project_setting_panel_.refresh_aa_setting();
        if(Project::get_active_project())
        {
            editor_setting_panel_.set_last_proj_path(rkproj_path);
            Path assets_dir{ Project::get_active_project()->get_assets_dir() };
            content_browser_panel_.set_context(assets_dir);
            AssetsManager::scan_assets_directory(assets_dir);

            auto scene{ Project::get_active_project()->get_active_scene() };
            update_current_scene(scene);
        }
    }

    void EditorLayer::open_project(const Window* window)
    {
        auto file_path{ FileDialogs::open_file
            (u8"Rocket Project (*.rkproj)|*.rkproj|", window) };
        if(file_path) {
            Path rkproj_path{ file_path.value() };
            open_project(rkproj_path);
        }
    }

    void EditorLayer::save_project() { Project::save_active(); }

    bool EditorLayer::on_key_pressed(KeyPressedEvent& e)
    {
        if(e.is_held()) return false;

        switch(e.get_key()) // function keys
        {
        case Key::F5:
            switch(scene_state_)
            {
            case SceneState::Edit:
                if(current_scene_) on_runtime_start();
                return true;
            case SceneState::Play:
                if(current_scene_) on_runtime_stop();
                return true;
            }
            return false;
        default: break;
        }

        if(scene_state_ == SceneState::Play) return false;

        bool ctrl{  Input::is_key_pressed(Key::RightControl)
                 || Input::is_key_pressed(Key::LeftControl)};
        bool shift{ Input::is_key_pressed(Key::RightShift  )
                 || Input::is_key_pressed(Key::LeftShift  )};

        switch(e.get_key())
        {
        case Key::Num1:
            editor_setting_panel_.set_gizmo_mode(Gizmo::Mode::Translate);
            return true;
        case Key::Num2:
            editor_setting_panel_.set_gizmo_mode(Gizmo::Mode::Rotate);
            return true;
        case Key::Num3:
            editor_setting_panel_.set_gizmo_mode(Gizmo::Mode::Scale);
            return true;
        case Key::N:
            if(ctrl) { new_project(); return true; }
            return false;
        case Key::O:
            if(ctrl) { open_project(e.get_window_title()); return true; }
            return false;
        case Key::S:
            if(ctrl) { save_project(); return true; }
            return false;
        case Key::Keypad5:
            editor_cam_.reset(); return true;
        case Key::Delete:
            if(current_scene_) current_scene_->destroy_selected_entity();
            return true;
        }
        return false;
    }

    bool EditorLayer::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        if(current_scene_ && scene_state_ == SceneState::Play)
        {
            current_scene_->on_mouse_scrolled_runtime(e);
            return true;
        }
        return editor_cam_.on_mouse_scrolled(e);
    }

    bool EditorLayer::on_mouse_button_pressed(MouseButtonPressedEvent& e)
    {
        if(!current_scene_) return false;
        if(scene_state_ == SceneState::Play) return false;
        if(!main_viewport_.is_focused()) return true;
        // Just select main-viewport, do not set selected-entity

        bool is_gizmo_over{ Gizmo::is_over() &&
            current_scene_->get_selected_entity().valid()};
        if(is_gizmo_over || Gizmo::is_using()) return false;

        if(e.get_mouse_button() == Mouse::Left) {
            current_scene_->set_selected_entity(hovering_id_);
            return true;
        }
        return false;
    }

    void EditorLayer::update_current_scene(Ref<Scene> scene)
    {
        current_scene_ = scene;
        scene_hierarchy_panel_.set_context(current_scene_.get());
        if(current_scene_) current_scene_->
            set_viewport(last_vp_size_.x, last_vp_size_.y);

    // clean-up
        hovering_id_ = -1;
        main_output_ = nullptr;
        cam_output_  = nullptr;
        main_renderer_.clean_up();
        cam_renderer_ .clean_up();
    }

    void EditorLayer::entity_right_click_popup_content(Scene* scene)
    {
        CORE_ASSERT(scene, u8"EditorLayer: Scene empty!");
        if(ImGui::MenuItem("Delete")) scene->destroy_selected_entity();
        if(ImGui::MenuItem("Copy")) {
            Entity copied{ scene->copy_selected_entity() };
            scene->set_selected_entity(copied);
        }
    }
}
