module;
module EditorLayer;

import Gizmo;
import FXAAEffect;

namespace {
    using namespace rke;
    static void entity_right_click_popup_content(Entity entity)
    {
        if(!entity.is_valid()) return;
        auto [handle, scene]{ entity.split() };
        
        if(ImGui::MenuItem("Copy"))
            scene->set_selected_entity(scene->copy_entity(handle).get_handle());
        if(ImGui::MenuItem("Create Child"))
            scene->set_selected_entity(entity.create_child().get_handle());
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) scene->destroy_entity(handle);
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

        scene_serializer_.set_deserialize_hook
        ([this](Scene& scene, const ConfigReader& reader)
            { editor_cam_.deserialize_from(reader); });

    // Effects
        auto selected{ create_scope<OutlineEffect>
        (
            u8"Selected Outline",
            [this]() -> bool
            {
                Entity selected{ current_scene() ?
                    current_scene()->get_selected_entity() : Entity{} };
                EntityHandle selected_id{ selected.is_valid() ?
                    selected.get_handle() : entity_handle_null };
                return !gizmo::is_using() && !in_main_viewport_dragging_
                    && selected_id != hovering_id_;
            }
        )};
        selected->set_context ({
            [this]() {
                std::vector<OutlineEffect::Target> targets{};
                if(!current_scene()) return targets;
                Scene& scene{ *current_scene() };
                Entity selected{ scene.get_selected_entity() };
                if(!selected.is_valid()) return targets;
                targets.push_back({ selected.get_mesh(), selected.get_world_transform().matrix });
                return targets;
            },
            [this]() -> glm::mat4
            {
                Scene* scene{ current_scene() };
                return editing() ? editor_cam_.get_view_proj() :
                    (scene ? scene->get_master_camera().get_view_proj() : glm::mat4(1.0f));
            }
        });

        auto hovering{ create_scope<OutlineEffect>
        (
            u8"Hovering Outline",
            [this]() -> bool
            {
                return !gizmo::is_using() && !mouse_blocked()
                    && !in_main_viewport_dragging_ && editing()
                    && main_viewport_->is_hovered();
            }
        )};
        hovering->set_context ({
            [this]() {
                std::vector<OutlineEffect::Target> targets{};
                if(!editing()) return targets;
                Scene& scene{ *scene_edit_ };
                const Entity hovered{ scene.get_entity(hovering_id_) };
                if(!hovered.is_valid()) return targets;
                targets.push_back({ hovered.get_mesh(), hovered.get_world_transform().matrix });
                return targets;
            },
            [this]() -> glm::mat4
            {
                Scene* scene{ current_scene() };
                return editing() ? editor_cam_.get_view_proj() :
                    (scene ? scene->get_master_camera().get_view_proj() : glm::mat4(1.0f));
            }
        });

        auto plane_boxes{ create_scope<OutlineEffect>(u8"Plane Box Outline") };
        plane_boxes->set_context ({
            [this]() -> std::vector<OutlineEffect::Target>
            {
                std::vector<OutlineEffect::Target> targets{};
                Scene* scene{ current_scene() };
                if(!scene) return targets;

                const Plane& plane{ scene->get_physics_plane() };
                scene->for_each_entity_with<BoxCollider2DComponent>
                ([&targets, &plane](Entity entity)
                {
                    PlaneBox box{ entity.box_of(plane) };
                    if(box.empty()) return;
                    const auto& bcc{ entity.get<BoxCollider2DComponent>() };
                    box.scale_by(bcc.size_scale);
                    targets.push_back({ &mesh::quad(), box.to_world(plane) });
                });
                return targets;
            },
            [this]() -> glm::mat4
            {
                Scene* scene{ current_scene() };
                return editing() ? editor_cam_.get_view_proj() :
                    (scene ? scene->get_master_camera().get_view_proj() : glm::mat4(1.0f));
            }
        });
        
        auto fxaa{ create_scope<FXAAEffect>(u8"Fxaa") };
        auto tone_mapping{ create_scope<ToneMapping>(u8"Tone Mapping") };

        Path filepath{ file::editor_dir() / u8"settings" / u8"editor.yaml" };
        file::check_to_create_dir(filepath);
        auto reader{ ConfigReader::create(filepath) };
        CORE_ASSERT(reader, u8"EditorLayer: Failed to create reader!");

        auto effects_reader{ reader->get_child(u8"Post-Process") };
        if(effects_reader)
        {
            selected->deserialize_from(*effects_reader);
            hovering->deserialize_from(*effects_reader);
            plane_boxes->deserialize_from(*effects_reader);
            fxaa->deserialize_from(*effects_reader);
            tone_mapping->deserialize_from(*effects_reader);
        }

        post_processor_.push_effect(std::move(plane_boxes));
        post_processor_.push_effect(std::move(selected));
        post_processor_.push_effect(std::move(hovering));
        post_processor_.push_effect(std::move(fxaa));
        post_processor_.push_effect(std::move(tone_mapping));

        cam_renderer_.set_samples(1);
        cam_post_processor_.push_effect(create_scope<ToneMapping>(u8"Tone Mapping"));

    // Panels
        editor_setting_panel_->load_from(*reader);
        scene_hierarchy_panel_.set_on_entity_node_render(entity_right_click_popup_content);

        Path assets_dir{ file::assets_dir() };
        content_browser_panel_->set_folder_icon(assets_dir / u8"icons" / u8"folder.png");
        content_browser_panel_->set_image_icon (assets_dir / u8"icons" / u8"image.png" );
        content_browser_panel_->set_file_icon  (assets_dir / u8"icons" / u8"file.png"  );
        content_browser_panel_->load_from(file::editor_dir() / u8"settings" / u8"content-browser.yaml");

    // Toolbar
        toolbar_.emplace_icon_button(u8"Play",
            assets_dir / u8"icons" / u8"play.png",
            [this](IconButton*) { on_runtime_start(); },
            [this]() { return editing(); }
        );

        toolbar_.emplace_icon_button(u8"Stop",
            assets_dir / u8"icons" / u8"stop.png",
            [this](IconButton*) { on_runtime_stop(); },
            [this]() { return testing(); }
        );

    // Viewports
        main_viewport_->set_viewport_callback([this](Viewport& self)
        {
        // Gizmo
            if(editing() && self.is_focused())
            {
                gizmo::on_render(*scene_edit_,
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
            } else in_main_viewport_dragging_ = false;

        // Entity Pop-up
            if(editing() && ImGui::BeginPopupContextWindow
                (0, ImGuiPopupFlags_MouseButtonRight))
            {
                if(ImGui::IsWindowAppearing() &&
                   scene_edit_ && scene_edit_->is_handle_valid(hovering_id_))
                {
                    in_entity_popup_ = true;
                    scene_edit_->set_selected_entity(hovering_id_);
                }
                if(in_entity_popup_)
                    entity_right_click_popup_content
                        (scene_edit_->get_selected_entity());
                else { // may modify
                    if(ImGui::MenuItem("Reload Scene"))
                        reload_scene_edit();
                    ImGui::Separator();
                    if(ImGui::MenuItem("Refresh Shaders"))
                    {
                        Renderer::refresh_shader();
                        FXAAEffect::refresh_shader();
                        ToneMapping::refresh_shader();
                        OutlineEffect::refresh_shaders();
                    }
                }
                ImGui::EndPopup();
            } else in_entity_popup_ = false;
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
        app().register_panel(&animation_editor_panel_);
        app().register_panel(&toolbar_, { .always_on = true });
    }

    void EditorLayer::on_detach()
    {
        if(testing()) on_runtime_stop();
        
        auto writer{ ConfigWriter::create() };
        CORE_ASSERT(writer, u8"EditorLayer: Failed to create writer!");

        writer->begin_map();
        editor_setting_panel_->save_to(*writer);

        writer->begin_map(u8"Post-Process");
        while(!post_processor_.empty())
        {
            Scope<PostProcessEffect> effect{ post_processor_.pop_effect() };
            CORE_ASSERT(effect, u8"EditorLayer: Effect Empty!");
            effect->serialize_to(*writer);
        }
        writer->end_map();
        
        writer->end_map();
        writer->push_to_file(file::editor_dir() / u8"settings" / u8"editor.yaml");

        app().unregister_panel(editor_setting_panel_.get());
        app().unregister_panel(content_browser_panel_.get());
        app().unregister_panel(main_viewport_.get());
        app().unregister_panel(cam_viewport_.get());

        app().unregister_panel(&scene_hierarchy_panel_);
        app().unregister_panel(&animation_editor_panel_);
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
            dispatcher.dispatch<ProjectLoadedEvent>([this](ProjectLoadedEvent& e)
            {
                attach_context(reinterpret_cast<Project*>(e.get_project()));
                return false; // do not block
            });
            dispatcher.dispatch<ProjectClearedEvent>([this](ProjectClearedEvent& e)
            {
                attach_context(nullptr);
                return false;
            });
            dispatcher.dispatch<ProjectSavedEvent>([this](ProjectSavedEvent& e)
            {
                if((void*)context_ == e.get_project())
                    on_context_project_saved();
                return false;
            });
            dispatcher.dispatch<ProjectSamplesSetEvent>([this](ProjectSamplesSetEvent& e)
            {
                if((void*)context_ == e.get_project())
                    on_context_project_samples_set(e.get_samples());
                return false;
            });
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
            editor_setting_panel_->set_gizmo_mode(gizmo::Mode::Translate);
            return true;
        case Key::Num2:
            editor_setting_panel_->set_gizmo_mode(gizmo::Mode::Rotate);
            return true;
        case Key::Num3:
            editor_setting_panel_->set_gizmo_mode(gizmo::Mode::Scale);
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

        bool is_gizmo_over{ gizmo::is_over() &&
            scene_edit_->get_selected_entity().is_valid() };
        if(is_gizmo_over || gizmo::is_using()) return false;

        if(e.get_mouse_button() == Mouse::Left)
        {
            if(scene_edit_->is_handle_valid(hovering_id_))
                scene_edit_->set_selected_entity(hovering_id_);
            else if(main_viewport_->is_focused())
                scene_edit_->set_selected_entity(entity_handle_null);
            return true;
        }
        return false;
    }

    void EditorLayer::attach_context(Project* project)
    {
        CORE_ASSERT(!testing(), u8"EditorLayer: Can't load project while testing!");
        
        editor_cams_.clear();
        scene_edit_ = nullptr;
        attach_scene(nullptr);

        context_ = project;
        content_browser_panel_->on_project_loaded(context_);
        animation_editor_panel_.on_project_loaded(context_);
    }

    void EditorLayer::on_context_project_saved()
        { save_scene_edit(); }

    void EditorLayer::on_context_project_samples_set(uint32 samples)
        { main_renderer_.set_samples(samples); }

    void EditorLayer::on_update(double dt)
    {
        RKE_PROFILE_FUNCTION();

        Layer::on_update(dt);
        if(editing()) editor_cam_.on_update(dt);

        ticker_.tick(dt, [this](double delta)
            { if(current_scene()) current_scene()->on_update(delta); }); // entity deleted here
    }

    void EditorLayer::on_render()
    {
        if(main_viewport_->resized())
        {
            glm::vec2 size{ main_viewport_->get_size() };
            auto w{ static_cast<uint32>(size.x) };
            auto h{ static_cast<uint32>(size.y) };

            editor_cam_.set_viewport_size(size);
            if(current_scene()) current_scene()->set_viewport(w, h);

            main_renderer_ .on_viewport_resized(w, h);
            post_processor_.on_viewport_resized(w, h);
        }
        if(cam_viewport_->resized())
        {
            glm::vec2 size{ cam_viewport_->get_size() };
            auto w{ static_cast<uint32>(size.x) };
            auto h{ static_cast<uint32>(size.y) };

            cam_renderer_.on_viewport_resized(w, h);
            cam_post_processor_.on_viewport_resized(w, h);
        }

        if(editing())
        {
            main_output_ = post_processor_.process
            (
                main_renderer_.render (
                    scene_edit_,
                    editor_cam_.get_view_proj(),
                    editor_cam_.calculate_pos()
                )
            );

            if(scene_edit_ && main_viewport_->is_hovered() &&
             ! in_main_viewport_dragging_ &&
             !(gizmo::is_over() && scene_edit_->get_selected_entity().is_valid()))
            {
                glm::vec2 vp_mouse{ main_viewport_->get_mouse_pos() };
                hovering_id_ = std::bit_cast<EntityHandle>
                    (main_renderer_.get_hovering_id(vp_mouse.x, vp_mouse.y));
            }
            else hovering_id_ = entity_handle_null;
        }
        else if(testing())
        {
            main_output_ = post_processor_.process
                (main_renderer_.render(scene_test_->get_master_camera()));
        }
        else { main_output_ = nullptr; }

        if(scene_edit_ && cam_viewport_->on() && current_scene()
         && !cam_viewport_->hidden() && cam_viewport_->visible())
        {
            Scene& scene{ *current_scene() };
            auto cv_size{ cam_viewport_->get_size() };
            scene.set_viewport(static_cast<uint32>(cv_size.x), static_cast<uint32>(cv_size.y));
            cam_output_ = cam_post_processor_.process
                (cam_renderer_.render(scene.get_demo_camera()));
            auto mv_size{ main_viewport_->get_size() };
            scene.set_viewport(static_cast<uint32>(mv_size.x), static_cast<uint32>(mv_size.y));
        }
        else cam_output_ = nullptr;
    }

    bool EditorLayer::should_block_mouse() { return editing(); }
    bool EditorLayer::should_block_keyboard() { return editing(); }

    void EditorLayer::on_runtime_start()
    {
        if(!editing()) return;

        scene_test_ = scene_edit_->duplicate();
        CORE_ASSERT(scene_test_, u8"EditorLayer: Failed to copy edit scene!");

        attach_scene(scene_test_.get());
        scene_test_->on_runtime_start();
    }

    void EditorLayer::on_runtime_stop()
    {
        if(!testing()) return;

        scene_test_->on_runtime_stop();
        
    // may modify
        scene_edit_->set_selected_entity(scene_test_->get_selected_entity().get_uuid());
        scene_edit_->set_demo_camera(scene_test_->get_demo_camera().get_uuid());

        attach_scene(scene_edit_);
        scene_test_.reset();
    }

    void EditorLayer::attach_scene(Scene* scene)
    {
        scene_hierarchy_panel_.set_context(scene);
        hovering_id_ = entity_handle_null;
        main_output_ = nullptr;
        cam_output_  = nullptr;
        main_renderer_.clean_up();
        cam_renderer_ .clean_up();
        post_processor_.clean_up();
        cam_post_processor_.clean_up();

        if(!scene) return;
        
        glm::vec2 size{ main_viewport_ ?
            main_viewport_->get_size() : glm::vec2(0.0f) };
        scene->set_viewport(size.x, size.y);
    }

    bool EditorLayer::load_scene_edit(const String& name)
    {
        if(!context_) {
            CORE_ERROR(u8"EditorLayer: Context project null!");
            clear_scene_edit();
            return false;
        }
        clear_scene_edit();
        scene_edit_ = context_->load_scene(name, scene_serializer_);
        if(scene_edit_)
        {
            auto it{ editor_cams_.find(scene_edit_->get_name()) };
            if(it != editor_cams_.end())
                editor_cam_.set_view_props(it->second);
        }
        else editor_cam_.reset();

        attach_scene(scene_edit_);
        return true;
    }

    void EditorLayer::save_scene_edit()
    {
        if(!scene_edit_ || !context_) return;
        if(editing()) context_->save_scene(*scene_edit_, scene_serializer_);
    }

    void EditorLayer::clear_scene_edit()
    {
        if(scene_edit_)
        {
            const String& name{ scene_edit_->get_name() };
            if(name.empty()) CORE_ERROR(u8"EditorLayer: Scene edit name empty!");
            else editor_cams_[name] = editor_cam_.get_view_props();
        }
        // not reloading the scene, only clear handle
        scene_edit_ = nullptr;
        attach_scene(nullptr);
    }

    void EditorLayer::unload_scene_edit()
    {
        if(editing()) {
            String name{ scene_edit_->get_name() };
            clear_scene_edit();
            CORE_ASSERT(context_, u8"EditorLayer: "
                u8"Context project can't be null since scene edit exists!")
            context_->remove_scene(name);
        }
    }

    void EditorLayer::reload_scene_edit()
    {
        String name{ scene_edit_->get_name() };
        unload_scene_edit();
        load_scene_edit(name);
    }

    Scene* EditorLayer::current_scene()
    {
        if(editing()) return scene_edit_;
        if(testing()) return scene_test_.get();
        return nullptr;
    }
}
