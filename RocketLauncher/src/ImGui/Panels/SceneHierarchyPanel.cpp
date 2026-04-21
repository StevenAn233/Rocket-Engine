module;
module SceneHierarchyPanel;

namespace rke
{
    void SceneHierarchyPanel::on_imgui_render()
    {
        if(!context_) {
            ImGui::Begin(get_name().raw());
            ImGui::End();
            ImGui::Begin("##expanded", nullptr, ImGuiWindowFlags_NoTitleBar);
            ImGui::End();
            return;
        }

        Entity selected{ context_->get_selected_entity() };
    // Scene Hierarchy
        ImGui::Begin(get_name().raw());

        ImGuiTreeNodeFlags flags
        {(is_scene_selected_ ? ImGuiTreeNodeFlags_Selected : 0)
        | ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_DefaultOpen};

        bool opened{ ImGui::TreeNodeEx(static_cast<void*>(context_), flags,
            (context_->to_save() ? "%s*" : "%s"), context_->get_name().raw())};
        if(ImGui::IsItemClicked()) {
            is_scene_selected_ = true;
            context_->set_selected_entity(Entity{});
        }

        if(opened) {
            std::vector<Entity> all_entities{ context_->get_all_entities() };
            for(Entity entity : all_entities)
                draw_entity_node(entity, selected);

            if(ImGui::IsWindowHovered()
            && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
            {
                is_scene_selected_ = false;
                context_->set_selected_entity(Entity{});
            }
            draw_entity_popup();

            ImGui::TreePop();
        }
        
        ImGui::End();

    // Expanded(Entity or Scene)
        ImGui::Begin("##expanded", nullptr, ImGuiWindowFlags_NoTitleBar);
        ImGui::PushID(get_name().raw());

        if(selected.valid()) {
            draw_components(selected);
            add_components_popup(selected);
        }
        else if(is_scene_selected_) draw_scene_settings();

        ImGui::PopID();
        ImGui::End();
    }

    void SceneHierarchyPanel::draw_entity_node(Entity entity, Entity selected)
    {
        ImGui::PushID(static_cast<int>(entity.get_handle()) + 1);
        const String& tag{ entity.get<TagComponent>().tag };
        ImGuiTreeNodeFlags flags {
        ((selected == entity) ? ImGuiTreeNodeFlags_Selected : 0)
          | ImGuiTreeNodeFlags_OpenOnArrow
          | ImGuiTreeNodeFlags_SpanAvailWidth
        }; // keep clicked entity selected

        bool opened{ ImGui::TreeNodeEx("entity_node", flags, "%s", tag.raw())};

        if(ImGui::IsItemClicked()) {
            is_scene_selected_ = false;
            context_->set_selected_entity(entity);
        }

        if(ImGui::BeginPopupContextItem()) {
            if(ImGui::IsWindowAppearing())
                context_->set_selected_entity(entity);
            on_entity_node_render_(context_);
            ImGui::EndPopup();
        }

        if(opened) {
            // draw something
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void SceneHierarchyPanel::draw_entity_popup()
    {
        constexpr ImGuiPopupFlags POPUP_FLAGS
        {
            ImGuiPopupFlags_MouseButtonRight |
            ImGuiPopupFlags_NoOpenOverItems
        };

        if(ImGui::BeginPopupContextWindow(0, POPUP_FLAGS))
        {
            if(ImGui::MenuItem("Create Entity"))
                context_->set_selected_entity(context_->create_entity());
            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::draw_scene_settings()
    {
        CORE_ASSERT(context_, u8"SceneHierarchy: No scene context set!");

        layout::tree_node_branch(u8"Name", [this]()
        {
            const String& name{ context_->get_name() };
            char buffer[256]{};
            strncpy(buffer, name.raw(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
            if(ImGui::InputText("##tag", buffer, sizeof(buffer)))
                context_->set_name(String(str::to_char8(buffer)));   
        });

        layout::tree_node_branch(u8"Physics", [this]()
        {
            static glm::vec2 recover{ Gravity2D::get_default() };
            context_->mark_modified_if(layout::drag_float2_control(u8"Gravity",
                context_->get_gravity_mut(), 0.01f, recover));
        });
    }

    void SceneHierarchyPanel::draw_components(Entity entity)
    {
        CORE_ASSERT(context_, u8"SceneHierarchy: No scene context set!");
        check_then_draw<TagComponent>(entity, u8"Tag", [this](Entity ent)
        {
            ImGui::PushID(static_cast<int>(ent.get_handle()) + 1);
            String& tag{ ent.get_mut<TagComponent>().tag };
            char buffer[256]{};
            strncpy(buffer, tag.raw(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
            if(ImGui::InputText("##tag", buffer, sizeof(buffer)))
            {
                tag = String(str::to_char8(buffer), strlen(buffer));
                context_->mark_modified();
            }
            ImGui::PopID();
        });

        check_then_draw<TransformComponent>(entity, u8"Transform", [&](Entity ent)
        {
            auto& transform_com{ ent.get_mut<TransformComponent>() };
            bool has_sprite{ ent.has<SpriteComponent>() };
            bool has_camera{ ent.has<CameraComponent>() };

            if(transform_com.locked) { context_->mark_modified_if(
                layout::drag_float3_control(u8"Position",
                    transform_com.position, 0.0f, glm::vec3(0.0f),
                    std::nullopt, std::nullopt, std::nullopt)
            );} else { context_->mark_modified_if(
                layout::drag_float3_control(u8"Position",
                    transform_com.position, 0.1f, glm::vec3(0.0f),
                    glm::vec2(0.0f), glm::vec2(0.0f), glm::vec2(0.0f))
            );}

            if(transform_com.locked) { context_->mark_modified_if(
                layout::drag_float3_control(u8"Rotition",
                    transform_com.rotation, 0.0f, glm::vec3(0.0f),
                    std::nullopt, std::nullopt, std::nullopt)
            );} else { context_->mark_modified_if(
                layout::drag_float3_control(u8"Rotation",
                    transform_com.rotation, 0.5f, glm::vec3(0.0f),
                    glm::vec2(0.0f), glm::vec2(0.0f), glm::vec2(0.0f))
            );}

            if(transform_com.locked || ent.has<CameraComponent>()) {
                context_->mark_modified_if(layout::drag_float3_control(
                    u8"Size", transform_com.size, 0.0f, glm::vec3(1.0f),
                    std::nullopt, std::nullopt, std::nullopt));
            } else if(ent.has<SpriteComponent>()) {
                context_->mark_modified_if(layout::drag_float3_control(
                    u8"Size", transform_com.size, 0.1f, glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec2(0.0f), glm::vec2(0.0f), std::nullopt));
            } else { context_->mark_modified_if(
                layout::drag_float3_control(u8"Size",
                     transform_com.size, 0.1f, glm::vec3(1.0f)));
            }
        });

        check_then_draw<CameraComponent>(entity, u8"Camera", [&](Entity ent)
        {
            auto& camera{ ent.get_mut<CameraComponent>().camera };

            layout::two_columns_table(u8"Projection", [&]()
            {
                constexpr const char* type_strs[]{ "Perspective", "Orthographic" };
                const char* current_type_str{ type_strs[camera.get_current_type_int()] };

                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width - 1.0f);
                if(ImGui::BeginCombo("##proj", current_type_str))
                {
                    for(int i{}; i < 2; i++)
                    {
                        bool selected{ current_type_str == type_strs[i] };
                        if(ImGui::Selectable(type_strs[i], selected))
                            { camera.set_current_type(i); context_->mark_modified(); }
                        if(selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            });

            if(camera.get_current_type() == SceneCamera::Type::Orthographic)
            {
                float current_size{ camera.get_orthographic_size() };
                if(layout::drag_float_control(u8"OrthoSize", current_size, 0.1f, 10.0f))
                    { camera.set_orthographic_size(current_size); context_->mark_modified(); }

                float current_near{ camera.get_orthographic_near_clip() };
                if(layout::drag_float_control(u8"NearClip", current_near, 0.1f, -10.0f))
                    { camera.set_orthographic_near_clip(current_near); context_->mark_modified(); }

                float current_far{ camera.get_orthographic_far_clip() };
                if(layout::drag_float_control(u8"FarClip", current_far, 0.1f, 10.0f))
                    { camera.set_orthographic_far_clip(current_far); context_->mark_modified(); }
            }
            else if(camera.get_current_type() == SceneCamera::Type::Perspective)
            {
                float current_fov{ camera.get_perspective_vertical_fov() };
                if(layout::drag_float_control(u8"Vert-Fov", current_fov, 0.5f, 45.0f))
                    { camera.set_perspective_vertical_fov(current_fov); context_->mark_modified(); }

                float current_near{ camera.get_perspective_near_clip() };
                if(layout::drag_float_control(u8"NearClip", current_near, 0.01f, 0.01f))
                    { camera.set_perspective_near_clip(current_near); context_->mark_modified(); }

                float current_far{ camera.get_perspective_far_clip() };
                if(layout::drag_float_control(u8"FarClip", current_far, 1.0f, 100.0f))
                    { camera.set_perspective_far_clip(current_far); context_->mark_modified(); }
            }
        });

        check_then_draw<SpriteComponent>(entity, u8"Sprite", [this](Entity ent)
        {
            auto& sc{ ent.get_mut<SpriteComponent>() };

        // Texture
            bool no_texture{ !sc.texture.has_asset() };
            constexpr ImGuiTreeNodeFlags tree_flags
            {   ImGuiTreeNodeFlags_SpanFullWidth
              | ImGuiTreeNodeFlags_AllowOverlap 
              | ImGuiTreeNodeFlags_FramePadding
              | ImGuiTreeNodeFlags_DrawLinesToNodes
              | ImGuiTreeNodeFlags_DefaultOpen
            };

            ImGui::PushID("TextureAssetNode");
            bool tex_opened{ false };
            if(no_texture) ImGui::Text("Texture:");
            else tex_opened = ImGui::TreeNodeEx("##TextureTree", tree_flags, "Texture:");
            ImGui::SameLine();

            float available_width{ ImGui::GetContentRegionAvail().x };
            String display_name{ no_texture ? u8"<No Texture>" :
                AssetsManager::get_asset_path(sc.texture.uuid).filename().string() };

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            if(ImGui::Button(display_name.raw(), ImVec2(available_width, 0.0f)))
            {
                sc.texture = {};
                context_->mark_modified();
            }
            ImGui::PopStyleColor();

            if(ImGui::BeginDragDropTarget()) { // Attached to button ^
                if(const auto* payload{ ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET") })
                {
                    AssetUUID dropped_uuid{ *reinterpret_cast<const AssetUUID*>(payload->Data) };
                    sc.texture = { dropped_uuid };
                    context_->mark_modified();
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::Columns(1);
        // ---

            if(tex_opened && sc.texture.is_loaded())
            {
                AssetSettings settings{ AssetsManager::get_asset_settings(sc.texture.uuid) };
                auto* tex_settings{ std::get_if<TextureSettings>(&settings) };
                CORE_ASSERT(tex_settings, u8"SceneHierarchyPanel: Texture setting format incorrect!");
                layout::two_columns_table(u8"Filter", [&]()
                {
                    static constexpr const char* items[]{ "Linear", "Nearest" };
                    int option{ static_cast<int>(tex_settings->filt) };

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if(ImGui::Combo("##filt", &option, items, (int)std::size(items)))
                    {
                        tex_settings->filt = static_cast<Texture::FiltFormat>(option);
                        sc.texture = { AssetsManager::
                            get_or_create_sub_uuid(sc.texture.uuid, *tex_settings) };
                        context_->mark_modified();
                    }
                });

                layout::two_columns_table(u8"Wrapping", [&]()
                {
                    static constexpr const char* items[]{ "Clamp to Edge", "Repeat" };
                    int option{ static_cast<int>(tex_settings->wrap) };
                
                    float available_width{ ImGui::GetContentRegionAvail().x };
                    ImGui::SetNextItemWidth(available_width);
                    if(ImGui::Combo("##wrap", &option, items, (int)std::size(items)))
                    {
                        tex_settings->wrap = static_cast<Texture::WrapFormat>(option);
                        sc.texture = { AssetsManager::
                            get_or_create_sub_uuid(sc.texture.uuid, *tex_settings) };
                        context_->mark_modified();
                    }
                });

                auto& ta{ sc.texture };
                auto tex{ AssetsManager::get_asset<Texture2D>(ta.handle) };
                context_->mark_modified_if(layout::drag_float_control(u8"Tiling",
                    ta.tiling_factor, 0.5f, 1.0f, glm::vec2(0.0f, 100.0f)));
                context_->mark_modified_if(layout::drag_float2_control(
                    u8"Cell Pixels", ta.cell_pixels, 1.0f, {
                        tex ? static_cast<float>(tex->get_width ()) : 1.0f,
                        tex ? static_cast<float>(tex->get_height()) : 1.0f
                    },
                    glm::vec2(1.0f, 4096.0f), glm::vec2(1.0f, 4096.0f), u8"%.0f"));
                context_->mark_modified_if(layout::drag_float2_control(
                    u8"Cell Coords", ta.cell_coords, 1.0f, { 0.0f, 0.0f },
                    glm::vec2(0.0f, 100.0f), glm::vec2(0.0f, 100.0f), u8"%.0f"));
                context_->mark_modified_if(layout::drag_float2_control(
                    u8"Cell Counts", ta.cell_counts, 1.0f, { 1.0f, 1.0f },
                    glm::vec2(1.0f, 100.0f), glm::vec2(1.0f, 100.0f), u8"%.0f"));
            }
            if(tex_opened) ImGui::TreePop();
            ImGui::PopID();

            layout::two_columns_table(u8"Color", [&]()
            {
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                static constexpr ImGuiColorEditFlags COLOR_EDIT_FLAGS
                {	ImGuiColorEditFlags_Float
                  | ImGuiColorEditFlags_InputRGB
                  | ImGuiColorEditFlags_AlphaBar
                };
                context_->mark_modified_if(ImGui::ColorEdit4
                    ("##color", glm::value_ptr(sc.color), COLOR_EDIT_FLAGS));
            });

            layout::two_columns_table(u8"Blending Mode", [&]()
            {
                constexpr const char* items[]{ "Opaque", "Cutout", "Transparent" };
                int option{ static_cast<int>(sc.blending_mode) };

                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                if(ImGui::Combo("##blending_mode", &option, items, (int)std::size(items)))
                {
                    sc.blending_mode = SpriteComponent::BlendingMode(option);
                    context_->mark_modified();
                }
            });

            layout::two_columns_table(u8"Render Layer", [&, this]()
            {
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                context_->mark_modified_if(ImGui::SliderInt
                    ("##rendering_layer", &sc.rendering_layer, 0, 31));
            });
        });

        check_then_draw<Rigidbody2DComponent>(entity, u8"Rigidbody 2D", [this](Entity ent)
        {
            auto& rbc{ ent.get_mut<Rigidbody2DComponent>() };

            layout::two_columns_table(u8"Body Type", [&]()
            {
                constexpr const char* items[]{ "Static", "Dynamic", "Kinematic" };
                int option{ static_cast<int>(rbc.type) };

                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                if(ImGui::Combo("##body_type", &option, items, (int)std::size(items)))
                {
                    rbc.type = static_cast<Rigidbody2DComponent::BodyType>(option);
                    context_->mark_modified();
                }
            });

            context_->mark_modified_if(ImGui::Checkbox
                ("Rotation Fixed", &rbc.rotation_fixed));
        });

        check_then_draw<BoxCollider2DComponent>(entity, u8"Box Collider 2D", [this](Entity ent)
        {
            auto& bcc{ ent.get_mut<BoxCollider2DComponent>() };

            layout::two_columns_table(u8"Physics Layer", [&]()
            {
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                uint8 index{ bcc.layer_index };
                auto& physics_layers{ Project::
                    get_active_project()->get_config_mut().physics_layers };
                if(ImGui::BeginCombo("##physics_layer", physics_layers.get_name(index).raw()))
                {
                    for(uint8 i{}; i < physics_layers.get_showed_layer_count(); i++)
                    {
                        ImGui::PushID(i);
                        bool is_selected{ i == index };
                        if(ImGui::Selectable(physics_layers.get_name(i).raw(), is_selected))
                            { bcc.layer_index = i; context_->mark_modified(); }
                        if(is_selected) ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                    }

                    ImGui::EndCombo();
                }
            });

            context_->mark_modified_if(layout::drag_float2_control(
                u8"Offset", bcc.offset, 0.01f, glm::vec2(0.0f, 0.0f),
                glm::vec2(-1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)));

            context_->mark_modified_if(layout::drag_float2_control(
                u8"Size", bcc.size, 0.01f, glm::vec2(0.5f, 0.5f),
                glm::vec2(0.01f, 1.0f), glm::vec2(0.01f, 1.0f)));

            context_->mark_modified_if(layout::drag_float_control(
                u8"Density", bcc.density, 0.10f, 1.0f, glm::vec2(0.0f, 100.0f)));
            context_->mark_modified_if(layout::drag_float_control(
                u8"Friction", bcc.friction, 0.01f, 0.5f, glm::vec2(0.0f, 2.0f)));
            context_->mark_modified_if(layout::drag_float_control(
                u8"Restitution", bcc.restitution, 0.01f, 0.0f, glm::vec2(0.0f, 1.0f)));
        });

        check_then_draw<NativeScriptComponent>(entity, u8"Native Script", [this](Entity ent)
        {
            auto& nsc{ ent.get_mut<NativeScriptComponent>() };
            const auto& script_registry{ ScriptRegistry::get() };

            const char* current_script_name{ "No Script" };
            if(!nsc.script_name.empty())
            {
                auto it{ script_registry.find(nsc.script_name) };
                if(it != script_registry.end())
                    current_script_name = it->first.raw();
                else current_script_name = "<Missing Script>";
            }
            // script_name  empty : No Script
            // script_name !empty && name  found : <Script Name>
            // script_name !empty && name !found : <Missing Script>

            if(ImGui::BeginCombo("##script", current_script_name))
            {
                bool is_none_selected{ nsc.script_name.empty() };
                if(ImGui::Selectable("No Script", is_none_selected))
                {
                    nsc.script_name.clear();
                    ScriptEngine::update_script(ent);
                    context_->mark_modified();
                }
                if(is_none_selected) ImGui::SetItemDefaultFocus();

                for(const auto& [name, _] : script_registry)
                {
                    bool is_selected{ nsc.script_name == name };
                    if(ImGui::Selectable(name.raw(), is_selected))
                    {
                        nsc.script_name = name;
                        ScriptEngine::update_script(ent);
                        context_->mark_modified();
                    }
                    if(is_selected) ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
        });
    }

    void SceneHierarchyPanel::add_components_popup(Entity selected)
    {
        static constexpr ImGuiPopupFlags POPUP_FLAGS
        {
            ImGuiPopupFlags_MouseButtonRight |
            ImGuiPopupFlags_NoOpenOverItems
        };

        bool popup_opened{ ImGui::BeginPopupContextWindow(0, POPUP_FLAGS) };
        if(!popup_opened) return;

        bool menu_opened{ ImGui::BeginMenu("Add Component") };
        if(!menu_opened){ ImGui::EndPopup(); return; }

        bool nothing_to_add{ true };
        ComponentRegistry::each([this, &selected, &nothing_to_add](auto type_id)
        {
            using Component = decltype(type_id)::Type;
            if(selected.has<Component>()) return;
            
            if constexpr(std::is_same_v<Component, CameraComponent>)
            {
                if(!selected.has<SpriteComponent>()) // !has_any_of<Sprite, Mesh>
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        selected.emplace<CameraComponent>();

                        context_->refresh_camera_components();
                        auto& tc{ selected.get_mut<TransformComponent>() };
                        tc.size = glm::vec3(1.0f);

                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else if constexpr(std::is_same_v<Component, SpriteComponent>)
            {
                if(!selected.has<CameraComponent>()) // !has_any_of<Cam, Mesh>
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        selected.emplace<SpriteComponent>();

                        auto& tc{ selected.get_mut<TransformComponent>() };
                        tc.size.z = 0.0f;

                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else if constexpr(std::is_same_v<Component, Rigidbody2DComponent>)
            {
                if(selected.has<SpriteComponent>())
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        selected.emplace<Rigidbody2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else if constexpr(std::is_same_v<Component, BoxCollider2DComponent>)
            {
                if(selected.has<Rigidbody2DComponent>())
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        selected.emplace<BoxCollider2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
            } else {
                nothing_to_add = false;
                if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                {
                    selected.emplace<Component>();
                    ImGui::CloseCurrentPopup();
                }
            }
        });
        if(nothing_to_add) { ImGui::TextColored
            (ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Nothing to add"); }

        ImGui::EndMenu();
        ImGui::EndPopup();
    }
}
