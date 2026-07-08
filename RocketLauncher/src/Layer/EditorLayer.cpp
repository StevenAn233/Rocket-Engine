module;
module EditorLayer;

import Gizmo;
import OutlineEffect;
import FXAAEffect;

namespace rke
{
    void EditorLayer::on_attach()
    {
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
                UUID id{ reader->get_at(u8"Cam Demo Target", 0ui64) };
                cam_renderer_.set_cam_demo_target(scene->get_entity(id));
            } else cam_renderer_.set_cam_demo_target({});
        });

    // Post-Processing Effects
        auto hovering{ create_scope<OutlineEffect>(u8"Hovering") };
        auto selected{ create_scope<OutlineEffect>(u8"Selected") };
        hovering->set_color(glm::vec4(1.0f, 0.8f, 0.0f, 1.0f));
        selected->set_color(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));

        editor_setting_panel_.set_hovering_handle(hovering.get());
        editor_setting_panel_.set_selected_handle(selected.get());
        editor_setting_panel_.load_from(file::editor_dir() / u8"settings" / u8"editor.yaml");

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
        Path assets_dir{ file::assets_dir() };
        content_browser_panel_.set_folder_icon(assets_dir / u8"icons" / u8"folder.png");
        content_browser_panel_.set_image_icon (assets_dir / u8"icons" / u8"image.png" );
        content_browser_panel_.set_file_icon  (assets_dir / u8"icons" / u8"file.png"  );
        content_browser_panel_.load_from(file::editor_dir() / u8"settings" / u8"content-browser.yaml");

    // Toolbar
        toolbar_.emplace_icon_button(u8"Play",
            Texture2D::create (
                assets_dir / u8"icons" / u8"play.png",
                Texture::FiltFormat::Linear,
                Texture::WrapFormat::Clamp2Edge, false),
            [this](IconButton*) { on_runtime_start(); },
            [this]() { return current_scene_ && editing(); });

        toolbar_.emplace_icon_button(u8"Stop",
            Texture2D::create (
                assets_dir / u8"icons" / u8"stop.png",
                Texture::FiltFormat::Linear,
                Texture::WrapFormat::Clamp2Edge, false),
            [this](IconButton*) { on_runtime_stop(); },
            [this]() { return current_scene_ && playing(); });

    // Viewports
        main_viewport_.set_viewport_callback([this](Viewport* self)
        {
        // Gizmo
            if(current_scene_ && editing() && self->is_focused())
            {
                Gizmo::on_render (
                    current_scene_->get_selected_entity(),
                    editor_setting_panel_.get_gizmo_mode(),
                    editor_cam_, mouse_blocked());
            }
        // Drag Drop
            if((!current_scene_ || editing()) && ImGui::BeginDragDropTarget())
            {
                in_main_viewport_dragging_ = true;
                const auto* payload{ ImGui::AcceptDragDropPayload("CONTENT_BROWSER_SCENE") };
                if(payload) {
                    String scene_path{ reinterpret_cast<const char8*>(payload->Data) };
                    if(Project::get_active_project() && scene_path.ends_with(u8".rkscene"))
                    {
                        Ref<Scene> active_scene{ Project::get_active_project()->load_scene(scene_path) };
                        update_current_scene(active_scene); // can be nullptr
                    }
                }
                ImGui::EndDragDropTarget();
            }
            else in_main_viewport_dragging_ = false;
        // Entity Pop-up
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
        main_viewport_.set_target_getter([this]() { return main_output_; });
        cam_viewport_ .set_target_getter([this]() { return cam_output_;  });

    // Modal
        project_creating_modal_.set_project_created_callback
        ([this](const Path& rkproj_path)
        {
            update_current_scene({});
            open_project(rkproj_path);
        });

    // PanelRegistry
        app().register_panel(&window_setting_panel_);
        app().register_panel(&editor_setting_panel_);
        app().register_panel(&scene_hierarchy_panel_);
        app().register_panel(&content_browser_panel_);
        app().register_panel(&project_setting_panel_);
        app().register_panel(&main_viewport_,
        {
            .always_on = true,
            .dont_block_when_hovered = true,
            .dont_block_when_focused = true
        });
        app().register_panel(&cam_viewport_);
        app().register_panel(&toolbar_, { .always_on = true });

    // ModalRegistry
        app().register_modal(&project_creating_modal_,
        {[this]() -> bool {
            if(to_create_new_proj_)
            {
                to_create_new_proj_ = false;
                return true;
            }
            return false;
        }});

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

        app().unregister_panel(&window_setting_panel_);
        app().unregister_panel(&editor_setting_panel_);
        app().unregister_panel(&scene_hierarchy_panel_);
        app().unregister_panel(&content_browser_panel_);
        app().unregister_panel(&project_setting_panel_);
        app().unregister_panel(&main_viewport_);
        app().unregister_panel(&cam_viewport_);
        app().unregister_panel(&toolbar_);

        if(Project::get_active_project())
            Project::get_active_project()->clear_active_scene();
        Project::clear_active();
    }

    void EditorLayer::on_event(Event& e)
    {
        if(e.handled()) return;
        EventDispatcher dispatcher{ e };
        dispatcher.dispatch<KeyPressedEvent>([this]
            (KeyPressedEvent& e) { return on_key_pressed(e); });
        dispatcher.dispatch<MouseScrolledEvent>([this]
            (MouseScrolledEvent& e) { return on_mouse_scrolled(e); });
        dispatcher.dispatch<MouseButtonPressedEvent>([this]
            (MouseButtonPressedEvent& e) { return on_mouse_button_pressed(e); });
    }

    bool EditorLayer::on_key_pressed(KeyPressedEvent& e)
    {
        if(e.is_held()) return false;
        if(!main_viewport_.is_focused()) return false;

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
            if(ctrl) { open_project(e.get_window_name()); return true; }
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
        if(!main_viewport_.is_hovered()) return false;
        if(current_scene_ && scene_state_ == SceneState::Play)
        {
            current_scene_->on_mouse_scrolled_runtime(e);
            return true;
        }
        return editor_cam_.on_mouse_scrolled(e);
    }

    bool EditorLayer::on_mouse_button_pressed(MouseButtonPressedEvent& e)
    {
        if(!main_viewport_.is_hovered()) return false;
        if(!current_scene_) return false;
        if(scene_state_ == SceneState::Play) return false;

        bool is_gizmo_over{ Gizmo::is_over() &&
            current_scene_->get_selected_entity().valid() };
        if(is_gizmo_over || Gizmo::is_using()) return false;

        if(e.get_mouse_button() == Mouse::Left) {
            current_scene_->set_selected_entity(hovering_id_);
            return true;
        }
        return false;
    }

    void EditorLayer::on_update(float dt)
    {
        RKE_PROFILE_FUNCTION();

        Layer::on_update(dt);

        if(main_viewport_.resized())
        {
            auto w{ static_cast<uint32>(main_viewport_.get_size().x) };
            auto h{ static_cast<uint32>(main_viewport_.get_size().y) };

            editor_cam_.set_viewport(w, h);
            if(current_scene_) current_scene_->set_viewport(w, h);

            main_renderer_.on_viewport_resized(w, h);
            editor_setting_panel_ .on_viewport_resized(w, h);
            project_setting_panel_.on_viewport_resized(w, h);
        }
        if(cam_viewport_.resized())
        {
            auto w{ static_cast<uint32>(cam_viewport_.get_size().x) };
            auto h{ static_cast<uint32>(cam_viewport_.get_size().y) };

            cam_renderer_.on_viewport_resized(w, h);
        }

        if(!current_scene_) return;

        bool hovering_enabled{ !Gizmo::is_using()
            && editor_setting_panel_.hovering_enabled_editor()
            && !in_main_viewport_dragging_ && editing()
            && !project_creating_modal_.in_use() && main_viewport_.is_hovered()
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
                glm::vec2 vp_mouse{ main_viewport_.get_mouse_pos() };
                hovering_id_ = main_renderer_.get_hovering_id(vp_mouse.x, vp_mouse.y);
            }
            else hovering_id_ = -1;

            if(current_scene_) {
                Entity target{ current_scene_->get_entity(hovering_id_, false) };
                editor_setting_panel_.get_hovering()->set_target(target);
            }

            if(current_scene_ && cam_viewport_.visible())
            {
                // switch to cam demo viewport size
                auto size{ cam_viewport_.get_size() };
                current_scene_->set_viewport(size.x, size.y);
                cam_output_ = cam_renderer_.cam_demo_render
                    (current_scene_.get(), current_scene_->get_selected_entity());
                current_scene_->set_viewport(size.x, size.y);
            }
            else cam_output_ = nullptr;
            break;
        case SceneState::Play:
            main_output_ = main_renderer_.on_render_runtime(current_scene_.get());
            cam_output_  = nullptr;
            break;
        default: break;
        }
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
        cam_viewport_.hide();
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
        cam_viewport_.show();

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

    void EditorLayer::update_current_scene(Ref<Scene> scene)
    {
        current_scene_ = scene;
        scene_hierarchy_panel_.set_context(current_scene_.get());
        auto size{ main_viewport_.get_size() };
        if(current_scene_) current_scene_->set_viewport(size.x, size.y);

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

    Window* EditorLayer::get_owner() { return reinterpret_cast<Window*>(owner_); }
}
