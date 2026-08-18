module;
module EditorLayer;

import Gizmo;
import FXAAEffect;

namespace {
    using namespace rke;
    static void entity_right_click_popup_content(Scene* scene)
    {
        if(!scene) return;
        if(ImGui::MenuItem("Delete")) scene->destroy_selected_entity();
        if(ImGui::MenuItem("Copy")) {
            Entity selected{ scene->get_selected_entity() };
            Entity copied{ scene->copy_entity(selected) };
            scene->set_selected_entity(copied);
        }
    }
}

namespace rke
{
    EditorLayer::EditorLayer(String name, Window* owner)
        : Layer(std::move(name), owner) {}

    void EditorLayer::on_attach()
    {
        editor_setting_panel_ = create_scope<EditorSettingPanel>(u8"Editor Settings", this);
        content_browser_panel_ = create_scope<ContentBrowserPanel>(u8"Content Browser");
        main_viewport_ = create_scope<Viewport>(u8"Main Viewport",
            [this]() { return main_output_; });
        cam_viewport_ = create_scope<Viewport>(u8"Camera Viewport",
            [this]() { return cam_output_; });

        scene_serializer_.set_serialize_hook
        ([this](const Scene& scene, ConfigWriter& writer)
            { editor_cam_.serialize_to(writer); });

    //  scene_serializer_.set_deserialize_hook
    //  ([this](Scene& scene, const ConfigReader& reader)
    //      { editor_cam_.deserialize_from(reader); });

    // Effects
        auto hovering{ create_scope<OutlineEffect>(u8"Hovering",
            [this]() -> bool {
                return !Gizmo::is_using() && !keyboard_blocked()
                    && editor_setting_panel_->hovering_enabled_editor()
                    && !in_main_viewport_dragging_ && editing()
                    && main_viewport_->is_hovered();
            }
        )};
        hovering->set_color(glm::vec4(1.0f, 0.8f, 0.0f, 1.0f));

        auto selected{ create_scope<OutlineEffect>(u8"Selected",
            [this]() -> bool {
                Entity selected{ current_scene() ?
                    current_scene()->get_selected_entity() : Entity{} };
                uint32 selected_id{ selected.valid() ? selected.get_handle() : entity_id_null };
                return !Gizmo::is_using()
                    && editor_setting_panel_->selected_enabled_editor()
                    && !in_main_viewport_dragging_
                    && (selected_id != hovering_id_ || !hovering_outline_->enabled());
            }
        )};
        selected->set_color(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));

        auto fxaa{ create_scope<FXAAEffect>(u8"Fxaa",
            [this]() -> bool {
                Project* project{ app().get_project() };
                if(project) return project->get_config()
                    .anti_aliasing == AntiAliasing::FXAA;
                return false;
            }
        )};

        hovering_outline_ = hovering.get();
        selected_outline_ = selected.get();

        Scene::set_on_entity_selected([this](Entity entity)
            { selected_outline_->set_target(entity); });

    // SceneRenderer
        main_renderer_.add_effect(std::move(hovering));
        main_renderer_.add_effect(std::move(selected));
        main_renderer_.add_effect(std::move(fxaa));
        cam_renderer_.set_samples(1);

    // EditorSetting
        editor_setting_panel_->load_from(file::editor_dir() / u8"settings" / u8"editor.yaml");

    // SceneHierarchy
        scene_hierarchy_panel_.set_on_entity_node_render(entity_right_click_popup_content);

    // ContentBrowser
        Path assets_dir{ file::assets_dir() };
        content_browser_panel_->set_folder_icon(assets_dir / u8"icons" / u8"folder.png");
        content_browser_panel_->set_image_icon (assets_dir / u8"icons" / u8"image.png" );
        content_browser_panel_->set_file_icon  (assets_dir / u8"icons" / u8"file.png"  );
        content_browser_panel_->load_from(file::editor_dir() / u8"settings" / u8"content-browser.yaml");

    // Toolbar
        toolbar_.emplace_icon_button(u8"Play",
            Texture2D::create (
                assets_dir / u8"icons" / u8"play.png",
                Texture::FiltFormat::Linear,
                Texture::WrapFormat::Clamp2Edge, false),
            [this](IconButton*) { on_runtime_start(); },
            [this]() { return editing(); }
        );

        toolbar_.emplace_icon_button(u8"Stop",
            Texture2D::create (
                assets_dir / u8"icons" / u8"stop.png",
                Texture::FiltFormat::Linear,
                Texture::WrapFormat::Clamp2Edge, false
            ),
            [this](IconButton*) { on_runtime_stop(); },
            [this]() { return testing(); }
        );

        toolbar_.emplace_icon_button(u8"Reload Script",
            Texture2D::create (
                assets_dir / u8"icons" / u8"refresh.png",
                Texture::FiltFormat::Linear,
                Texture::WrapFormat::Clamp2Edge, false
            ),
            [this](IconButton*) {
                Project* project{ app().get_project() };
                if(project) project->scripts_hot_reloading();
            },
            [this]() { return editing(); }
        );

    // Viewports
        main_viewport_->set_viewport_callback([this](Viewport& self)
        {
        // Gizmo
            if(editing() && self.is_focused())
            {
                Gizmo::on_render (
                    scene_edit_->get_selected_entity(),
                    editor_setting_panel_->get_gizmo_mode(),
                    editor_cam_, mouse_blocked());
            }
        // Drag Drop
            if(!testing() && ImGui::BeginDragDropTarget())
            {
                in_main_viewport_dragging_ = true;
                const auto* payload{ ImGui::AcceptDragDropPayload("CONTENT_BROWSER_SCENE") };
                if(payload) {
                    String scene_name{ reinterpret_cast<const char8*>(payload->Data) };
                    load_scene_edit(scene_name);
                }
                ImGui::EndDragDropTarget();
            }
            else in_main_viewport_dragging_ = false;
        // Entity Pop-up
            static bool in_popup{ false };
            if(!scene_edit_ || (hovering_id_ == entity_id_null && !in_popup)) return;
            if(ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight))
            {
                if(ImGui::IsWindowAppearing()) {
                    in_popup = true;
                    scene_edit_->set_selected_entity(hovering_id_);
                }
                entity_right_click_popup_content(scene_edit_);
                ImGui::EndPopup();
            }
            else in_popup = false;
        });

    // PanelRegistry
        app().register_panel(editor_setting_panel_.get());
        app().register_panel(content_browser_panel_.get());
        app().register_panel(main_viewport_.get(),
        {
            .always_on = true,
            .dont_block_when_hovered = true,
            .dont_block_when_focused = true
        });
        app().register_panel(cam_viewport_.get());

        app().register_panel(&scene_hierarchy_panel_);
        app().register_panel(&toolbar_, { .always_on = true });
    }

    void EditorLayer::on_detach()
    {
        if(testing()) on_runtime_stop();
        
        app().unregister_panel(editor_setting_panel_.get());
        app().unregister_panel(content_browser_panel_.get());
        app().unregister_panel(main_viewport_.get());
        app().unregister_panel(cam_viewport_.get());

        app().unregister_panel(&scene_hierarchy_panel_);
        app().unregister_panel(&toolbar_);

        editor_setting_panel_.reset();
        content_browser_panel_.reset();
        main_viewport_.reset();
        cam_viewport_.reset();

        clear_scene_edit();
        app().clear_project();
    }

    void EditorLayer::on_event(Event& e)
    {
        if(e.handled()) return;
        EventDispatcher dispatcher{ e };
        if(e.belongs_to(EventCategoryProject))
        {
            dispatcher.dispatch<ProjectLoadedEvent>([this]
                (ProjectLoadedEvent& e) { return on_project_loaded(e); });
            dispatcher.dispatch<ProjectSavedEvent>([this]
                (ProjectSavedEvent& e) { return on_project_saved(e); });
            dispatcher.dispatch<ProjectSamplesSetEvent>([this]
                (ProjectSamplesSetEvent& e) { return on_project_samples_set(e); });
        }
        else if(e.belongs_to(EventCategoryInput))
        {
            dispatcher.dispatch<KeyPressedEvent>([this]
                (KeyPressedEvent& e) { return on_key_pressed(e); });
            dispatcher.dispatch<MouseScrolledEvent>([this]
                (MouseScrolledEvent& e) { return on_mouse_scrolled(e); });
            dispatcher.dispatch<MouseButtonPressedEvent>([this]
                (MouseButtonPressedEvent& e) { return on_mouse_button_pressed(e); });
        }
    }

    bool EditorLayer::on_key_pressed(KeyPressedEvent& e)
    {
        if(e.is_held() || !scene_edit_
         || !main_viewport_->is_focused()) return false;

        if(e.get_key() == Key::F5) {
            if(editing()) { on_runtime_start(); return true; }
            if(testing()) { on_runtime_stop (); return true; }
            return false;
        }

        if(testing()) return false;

        switch(e.get_key())
        {
        case Key::Num1:
            editor_setting_panel_->set_gizmo_mode(Gizmo::Mode::Translate);
            return true;
        case Key::Num2:
            editor_setting_panel_->set_gizmo_mode(Gizmo::Mode::Rotate);
            return true;
        case Key::Num3:
            editor_setting_panel_->set_gizmo_mode(Gizmo::Mode::Scale);
            return true;
        case Key::Keypad5:
            editor_cam_.reset();
            return true;
        case Key::Delete:
            scene_edit_->destroy_selected_entity();
            return true;
        }
        return false;
    }

    bool EditorLayer::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        if(!scene_edit_ || !main_viewport_->is_hovered()) return false;
        if(testing()) {
            scene_test_->on_mouse_scrolled_runtime(e);
            return true;
        }
        return editor_cam_.on_mouse_scrolled(e);
    }

    bool EditorLayer::on_mouse_button_pressed(MouseButtonPressedEvent& e)
    {
        if(!main_viewport_->is_hovered()) return false;
        if(!scene_edit_ || testing()) return false;

        bool is_gizmo_over{ Gizmo::is_over() &&
            scene_edit_->get_selected_entity().valid() };
        if(is_gizmo_over || Gizmo::is_using()) return false;

        if(e.get_mouse_button() == Mouse::Left) {
            scene_edit_->set_selected_entity(hovering_id_);
            return true;
        }
        return false;
    }

    bool EditorLayer::on_project_loaded(ProjectLoadedEvent& e)
    {
        CORE_ASSERT(!testing(), u8"EditorLayer: Can't load project while testing!");
        clear_scene_edit();
        content_browser_panel_->on_project_loaded();
        return true;
    }

    bool EditorLayer::on_project_saved(ProjectSavedEvent& e)
    {
        save_scene_edit();
        return true;
    }

    bool EditorLayer::on_project_samples_set(ProjectSamplesSetEvent& e)
    {
        uint32 samples{ e.get_samples() };
        main_renderer_.set_samples(samples);
        editor_setting_panel_->set_outline_samples(samples);
        return true;     
    }

    void EditorLayer::on_update(float dt)
    {
        RKE_PROFILE_FUNCTION();

        Layer::on_update(dt);

        if(main_viewport_->resized())
        {
            auto w{ static_cast<uint32>(main_viewport_->get_size().x) };
            auto h{ static_cast<uint32>(main_viewport_->get_size().y) };

            editor_cam_.set_viewport(w, h);
            if(current_scene()) current_scene()->set_viewport(w, h);
            main_renderer_.on_viewport_resized(w, h);
        }
        if(cam_viewport_->resized())
        {
            auto w{ static_cast<uint32>(cam_viewport_->get_size().x) };
            auto h{ static_cast<uint32>(cam_viewport_->get_size().y) };

            cam_renderer_.on_viewport_resized(w, h);
        }

        if(editing()) editor_cam_.on_update(dt);
        if(current_scene()) current_scene()->on_update(dt); // entity deleted here
    }

    void EditorLayer::on_render()
    {
        if(editing())
        {
            main_output_ = main_renderer_.render
            (
                scene_edit_,
                editor_cam_.get_view_proj(),
                editor_cam_.get_pos()
            );

            if(scene_edit_ && main_viewport_->is_hovered() &&
             !(Gizmo::is_over() && scene_edit_->get_selected_entity().valid()))
            {
                glm::vec2 vp_mouse{ main_viewport_->get_mouse_pos() };
                hovering_id_ = static_cast<uint32>(main_renderer_
                    .get_hovering_id(vp_mouse.x, vp_mouse.y));
            }
            else hovering_id_ = entity_id_null;

            if(scene_edit_) {
                Entity target{ scene_edit_->get_entity(hovering_id_) };
                hovering_outline_->set_target(target);
            }

            if(scene_edit_ && cam_viewport_->on() &&
              !cam_viewport_->hidden() && cam_viewport_->visible())
            {
                // switch to cam demo viewport size
                auto size{ cam_viewport_->get_size() };
                scene_edit_->set_viewport(size.x, size.y);
                cam_output_ = cam_renderer_.render_demo_cam(scene_edit_);
                scene_edit_->set_viewport(size.x, size.y);
            }
            else cam_output_ = nullptr;
        }
        else if(testing())
        {
            main_output_ = main_renderer_.render_master_cam(scene_test_.get());
            cam_output_  = nullptr;
        }
        else { main_output_ = cam_output_ = nullptr; }
    }

    bool EditorLayer::should_block_mouse() { return editing(); }
    bool EditorLayer::should_block_keyboard() { return editing(); }

    void EditorLayer::on_runtime_start()
    {
        if(!editing()) return;

        scene_test_ = scene_edit_->deep_copy(true);
        attach_scene(scene_test_.get());

        CORE_ASSERT(scene_test_, u8"EditorLayer: Failed to copy edit scene!");
        scene_test_->set_selected_entity(scene_edit_->get_selected_entity().get_uuid());

        cam_viewport_->hide();
        scene_test_->on_runtime_start();
    }

    void EditorLayer::on_runtime_stop()
    {
        if(!testing()) return;

        scene_test_->on_runtime_stop();
        cam_viewport_->show();

        scene_edit_->set_selected_entity(scene_test_->get_selected_entity().get_uuid());

        attach_scene(scene_edit_);
        scene_test_.reset();
    }

    bool EditorLayer::load_scene_edit(const String& name)
    {
        if(!app().get_project()) {
            CORE_ERROR(u8"EditorLayer: No project loaded!");
            clear_scene_edit();
            return false;
        }
        scene_edit_ = app().get_project()->load_scene(name, scene_serializer_);
    // TO MODIFY
        Scope<ConfigReader> reader{ ConfigReader::create(scene_edit_->get_path()) };
        editor_cam_.deserialize_from(*reader);
    // TO MODIFY
        attach_scene(scene_edit_);
        return true;
    }

    void EditorLayer::save_scene_edit()
    {
        if(!scene_edit_) return;
        if(editing() && app().get_project())
            app().get_project()->save_scene(*scene_edit_, scene_serializer_);
    }

    void EditorLayer::clear_scene_edit()
    {
        scene_edit_ = nullptr;
        attach_scene(nullptr);
    }

    void EditorLayer::attach_scene(Scene* scene)
    {
        scene_hierarchy_panel_.set_context(scene);

        if(scene) {
            glm::vec2 size{ main_viewport_ ?
                main_viewport_->get_size() : glm::vec2(0.0f) };
            scene->set_viewport(size.x, size.y);
        }

        hovering_id_ = entity_id_null;
        main_output_ = nullptr;
        cam_output_  = nullptr;
        main_renderer_.clean_up();
        cam_renderer_ .clean_up();
    }

    Scene* EditorLayer::current_scene()
    {
        if(editing()) return scene_edit_;
        if(testing()) return scene_test_.get();
        return nullptr;
    }
}
