module;
module SceneHierarchyPanel;

import Log;
import Animation;
import FileUtils;
import Texture;
import PhysicsLayers;
import Application;
import Project;
import AssetsManager;
import ScriptManager;

namespace {
    constexpr const char* entity_drag_payload{ "SCENE_ENTITY" };
}

namespace rke
{
    SceneHierarchyPanel::SceneHierarchyPanel(String name)
        : Panel(std::move(name)) {}
    
    void SceneHierarchyPanel::on_imgui_render()
    {
        if(!context_)
        {
            ImGui::Begin(get_name().raw());
            ImGui::End();
            ImGui::Begin("##expanded", nullptr, ImGuiWindowFlags_NoTitleBar);
            ImGui::End();
            return;
        }

    // Scene Hierarchy
        ImGui::Begin(get_name().raw());

        ImGuiTreeNodeFlags flags
        {(is_scene_selected_ ? ImGuiTreeNodeFlags_Selected : 0)
        | ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_DefaultOpen };

        bool opened{ ImGui::TreeNodeEx
        (
            static_cast<void*>(context_), flags,
            (context_->to_save() ? "%s*" : "%s"),
            context_->get_name().raw()
        )};
        clear_drop();
        
        if(ImGui::IsItemClicked()) set_scene_node_selected();
        draw_scene_node_drop_target(); // move entity to root tail
        
        if(opened) {
            bool entity_created{ false };
            draw_entity_popup(entity_created);

            Entity selected{ context_->get_selected_entity() };
            if(selected.is_valid()) is_scene_selected_ = false;

            const auto [root_list, root_count]{ context_->get_roots() };
            const std::vector<EntityHandle> snapshot{ root_list, root_list + root_count };
            for(EntityHandle handle : snapshot) draw_entity_node(handle);
            draw_root_tail_drop_target();

            if(entity_created) ImGui::SetScrollHereY(1.0f); // very bottom

            if(ImGui::IsWindowHovered() && 
               ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
                set_entity_node_selected(Entity{});
            
            ImGui::TreePop();
        }
        apply_pending_drop();

        ImGui::End();

    // Expanded(Entity or Scene)
        ImGui::PushID(get_name().raw());
        ImGui::Begin("Selected", nullptr);
        
        Entity selected{ context_->get_selected_entity() };
        if(selected.is_valid())
        {
            draw_components(selected);
            add_components_popup(selected);
        }
        else if(is_scene_selected_) draw_scene_settings();

        ImGui::End();
        ImGui::PopID();
    }

    void SceneHierarchyPanel::draw_entity_node(EntityHandle handle)
    {
        Entity entity{ context_->get_entity(handle) };
        if(!entity.is_valid()) return;

        ImGui::PushID(static_cast<int>(handle) + 1);
        const char8* tag{ entity.get<IdentityComponent>().tag };

        const bool is_leaf{ !entity.has_any_child() };
        ImGuiTreeNodeFlags flags {
        (entity == context_->get_selected_entity() ? ImGuiTreeNodeFlags_Selected : 0)
          | ImGuiTreeNodeFlags_OpenOnArrow
          | ImGuiTreeNodeFlags_SpanAvailWidth
          | ImGuiTreeNodeFlags_DefaultOpen // may modify
        }; // keep clicked entity selected
        if(is_leaf) flags |= (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);

        bool opened{ ImGui::TreeNodeEx
        (
            "entity_node", flags,
            "%s", reinterpret_cast<const char*>(tag)
        )};

        if(ImGui::IsItemClicked()) set_entity_node_selected(entity);

        if(ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(entity_drag_payload, &handle, sizeof(EntityHandle));

            ImGui::TextUnformatted(reinterpret_cast<const char*>(tag)); // drag preview
            ImGui::EndDragDropSource();
        }

        draw_entity_drop_target(handle);

        if(ImGui::BeginPopupContextItem())
        {
            if(ImGui::IsWindowAppearing()) set_entity_node_selected(entity);
            on_entity_node_render_(context_->get_selected_entity());
            ImGui::EndPopup();
        }

        if(opened && !is_leaf)
        {
            const auto [kids, count]{ entity.get_children() };
            const std::vector<EntityHandle> snapshot{ kids, kids + count };
            for(EntityHandle child : snapshot) draw_entity_node(child);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    // dropping on a row: the top strip inserts before that node, the rest makes it a child
    void SceneHierarchyPanel::draw_entity_drop_target(EntityHandle handle)
    {
        if(!ImGui::BeginDragDropTarget()) return;

        const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload
        (
            entity_drag_payload,
            ImGuiDragDropFlags_AcceptPeekOnly |
            ImGuiDragDropFlags_AcceptNoPreviewTooltip
        )};
        if(!payload) { ImGui::EndDragDropTarget(); return; }

        const ImVec2 min { ImGui::GetItemRectMin () };
        const ImVec2 size{ ImGui::GetItemRectSize() };
        const bool as_sibling{ ImGui::GetIO().MousePos.y - min.y < size.y * 0.25f };

        ImDrawList* draw_list{ ImGui::GetWindowDrawList() };
        if(as_sibling) {
        // a line along the top edge: it would land just before this node
            draw_list->AddLine
            (
                ImVec2(min.x, min.y), ImVec2(min.x + size.x, min.y),
                ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0f
            );
        } else {
        // the whole row lights up: it would become a child of this node
            draw_list->AddRectFilled
            (
                min, ImVec2(min.x + size.x, min.y + size.y),
                ImGui::GetColorU32(ImGuiCol_DragDropTarget, 0.25f)
            );
        }
        if(!payload->IsDelivery()) { ImGui::EndDragDropTarget(); return; }

        const EntityHandle dragged{ *static_cast<const EntityHandle*>(payload->Data) };
        drop_child_ = dragged;

        if(!as_sibling) {
            drop_parent_ = handle;
            drop_before_ = entity_handle_null; // a new child goes last
        } else {
        // before this node, so under this node's parent
            drop_parent_ = context_->get_parent(handle).get_handle();
            drop_before_ = handle;
        }
        ImGui::EndDragDropTarget();
    }

    void SceneHierarchyPanel::draw_scene_node_drop_target()
    {
        if(!ImGui::BeginDragDropTarget()) return;

        const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload
        (
            entity_drag_payload,
            ImGuiDragDropFlags_AcceptPeekOnly |
            ImGuiDragDropFlags_AcceptNoPreviewTooltip
        )};
        if(payload) {
            const ImVec2 min { ImGui::GetItemRectMin () };
            const ImVec2 size{ ImGui::GetItemRectSize() };
            ImGui::GetWindowDrawList()->AddRectFilled
            (
                min, ImVec2(min.x + size.x, min.y + size.y),
                ImGui::GetColorU32(ImGuiCol_DragDropTarget, 0.25f)
            );

            if(payload->IsDelivery())
            {
                drop_child_  = *static_cast<const EntityHandle*>(payload->Data);
                drop_parent_ = entity_handle_null; // no parent: is root
                drop_before_ = entity_handle_null;
            }
        }
        ImGui::EndDragDropTarget();
    }

    void SceneHierarchyPanel::draw_root_tail_drop_target()
    {
        if(!ImGui::GetDragDropPayload()) return;

        const float width{ ImGui::GetContentRegionAvail().x };
        if(width <= 1.0f) return;

        const ImVec2 pos{ ImGui::GetCursorScreenPos() };
        ImGui::InvisibleButton("##root_tail", ImVec2(width, ImGui::GetTextLineHeight()));

        if(!ImGui::BeginDragDropTarget()) return;

        const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload
        (
            entity_drag_payload,
            ImGuiDragDropFlags_AcceptPeekOnly |
            ImGuiDragDropFlags_AcceptNoPreviewTooltip
        )};
        if(payload) {
            ImGui::GetWindowDrawList()->AddLine
            (
                pos, ImVec2(pos.x + width, pos.y),
                ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0f
            );

            if(payload->IsDelivery())
            {
                drop_child_  = *static_cast<const EntityHandle*>(payload->Data);
                drop_parent_ = entity_handle_null;
                drop_before_ = entity_handle_null;
            }
        }
        ImGui::EndDragDropTarget();
    }

    void SceneHierarchyPanel::apply_pending_drop()
    {
        if(context_->is_handle_null (drop_child_) ||
          !context_->is_handle_valid(drop_child_)) goto clear_end;
        context_->set_parent(drop_child_, drop_parent_, drop_before_);
    clear_end:
        clear_drop();
    }

    void SceneHierarchyPanel::clear_drop()
    {
        drop_child_  = entity_handle_null;
        drop_parent_ = entity_handle_null;
        drop_before_ = entity_handle_null;
    }

    void SceneHierarchyPanel::draw_entity_popup(bool& entity_created)
    {
        constexpr ImGuiPopupFlags POPUP_FLAGS
        {
            ImGuiPopupFlags_MouseButtonRight |
            ImGuiPopupFlags_NoOpenOverItems
        };

        if(ImGui::BeginPopupContextWindow(0, POPUP_FLAGS))
        {
            if(ImGui::MenuItem("Create Entity"))
            {
                set_entity_node_selected(context_->create_entity());
                entity_created = true;
            }
            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::draw_scene_settings()
    {
        layout::tree_node_branch<u8"Name">([this]()
        {
            const String& name{ context_->get_name() };
            char name_buffer[256]{};
            std::memcpy(name_buffer, name.raw(),
                std::min(name.length(), sizeof(name_buffer) - 1));
            if(ImGui::InputText (
                "##tag", name_buffer, sizeof(name_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue
            )) context_->set_name(String(str::to_char8(name_buffer)));
        });

        if(auto* physics_engine{ context_->physics_engine_.get() })
        {
            layout::tree_node_branch<u8"Physics">([this, physics_engine]()
            {
                context_->mark_modified_if (
                    layout::drag_float3_control<u8"Gravity">
                    (
                        physics_engine->get_gravity().ref(),
                        0.01f, Gravity::default_val()
                    )
                );
                glm::vec3 axis{ physics_engine->get_plane_axis() };
                if(layout::drag_float3_control<u8"Plane Axis">
                    (axis, 0.01f, glm::vec3(0.0f, 0.0f, 1.0f)))
                { context_->mark_modified(); physics_engine->set_plane(axis); }
            });
        }
    }

    void SceneHierarchyPanel::draw_components(Entity entity)
    {
        check_then_draw<IdentityComponent, u8"Tag">(entity, [this](Entity ent)
        {
            auto& ic{ ent.get_mut<IdentityComponent>() };
            char name_buffer[ic.tag_size]{};
            std::memcpy(name_buffer, &ic.tag[0],
                std::min(StringView(ic.tag).size(), ic.tag_size - 1));
            if(ImGui::InputText("##tag", name_buffer,
                sizeof(name_buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                ic.set_tag(StringView(str::to_char8(name_buffer)));
                context_->mark_modified();
            }
        });

        check_then_draw<TransformComponent, u8"Transform">(entity, [this](Entity ent)
        {
            auto& tc{ ent.get_mut<TransformComponent>() };

            bool translated{ layout::drag_float3_control
                <u8"Translation">(tc.translation, 0.1f, glm::vec3(0.0f)) };
            if(ent.has<Rigidbody2DComponent>() && translated)
            {
                auto& rbc{ ent.get_mut<Rigidbody2DComponent>() };
                // only dynamic bodies are impacted by forces
                if(rbc.type == BodyType::Dynamic) rbc.velocity = {};
            }
            context_->mark_modified_if(translated);

            glm::vec4 rotation{ tc.rotation.x, tc.rotation.y, tc.rotation.z, tc.rotation.w };
            if(layout::drag_float4_control<u8"Rotation">(rotation, 0.01f,
                glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f },
                glm::vec2{ -1.0f, 1.0f }, glm::vec2{ -1.0f, 1.0f },
                glm::vec2{ -1.0f, 1.0f }, glm::vec2{ -1.0f, 1.0f },
                u8"%.3f"))
            {
                if(glm::dot(rotation, rotation) > 1e-8f)
                {
                    tc.rotation = glm::normalize
                        (glm::quat{ rotation.w, glm::vec3{ rotation } });
                    context_->mark_modified();
                }
            }

            context_->mark_modified_if (
                layout::drag_float3_control<u8"Shear">
                    (tc.shear, 0.01f, glm::vec3(0.0f))
            );
            
            context_->mark_modified_if (
                layout::drag_float3_control<u8"Scale">
                    (tc.scale, 0.1f, glm::vec3(1.0f))
            );
            
            context_->mark_modified_if (
                layout::drag_float3_control<u8"Anchor">
                    (tc.anchor, 0.01f, glm::vec3(0.0f))
            );
        });

        check_then_draw<CameraComponent, u8"Camera">(entity, [this](Entity ent)
        {
            auto& camera{ ent.get_mut<CameraComponent>().camera };

            layout::two_columns_table<u8"Projection">([&]()
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
                if(layout::drag_float_control<u8"OrthoSize">(current_size, 0.1f, 10.0f))
                    { camera.set_orthographic_size(current_size); context_->mark_modified(); }

                float current_near{ camera.get_orthographic_near_clip() };
                if(layout::drag_float_control<u8"NearClip">(current_near, 0.1f, -10.0f))
                    { camera.set_orthographic_near_clip(current_near); context_->mark_modified(); }

                float current_far{ camera.get_orthographic_far_clip() };
                if(layout::drag_float_control<u8"FarClip">(current_far, 0.1f, 10.0f))
                    { camera.set_orthographic_far_clip(current_far); context_->mark_modified(); }
            }
            else if(camera.get_current_type() == SceneCamera::Type::Perspective)
            {
                float current_fov{ camera.get_perspective_vertical_fov() };
                if(layout::drag_float_control<u8"Vert-Fov">(current_fov, 0.5f, 45.0f))
                    { camera.set_perspective_vertical_fov(current_fov); context_->mark_modified(); }

                float current_near{ camera.get_perspective_near_clip() };
                if(layout::drag_float_control<u8"NearClip">(current_near, 0.01f, 0.01f))
                    { camera.set_perspective_near_clip(current_near); context_->mark_modified(); }

                float current_far{ camera.get_perspective_far_clip() };
                if(layout::drag_float_control<u8"FarClip">(current_far, 1.0f, 100.0f))
                    { camera.set_perspective_far_clip(current_far); context_->mark_modified(); }
            }
        });

        check_then_draw<SpriteComponent, u8"Sprite">(entity, [this](Entity ent)
        {
            auto& sc{ ent.get_mut<SpriteComponent>() };

            layout::two_columns_table<u8"Color">([&]()
            {
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                constexpr ImGuiColorEditFlags flags
                {	ImGuiColorEditFlags_Float
                  | ImGuiColorEditFlags_InputRGB
                  | ImGuiColorEditFlags_AlphaBar
                };
                context_->mark_modified_if(ImGui::ColorEdit4
                    ("##color", glm::value_ptr(sc.color), flags));
            });

            layout::two_columns_table<u8"Blending Mode">([&]()
            {
                constexpr const char* items[]{ "Opaque", "Transparent" };
                int option{ static_cast<int>(sc.blending_mode) };

                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                if(ImGui::Combo("##blending_mode", &option, items, (int)std::size(items)))
                {
                    sc.blending_mode = static_cast<BlendingMode>(option);
                    sc.rendering_layer = 0;
                    context_->mark_modified();
                }
            });

            layout::two_columns_table<u8"Render Layer">([&, this]()
            {
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                ImGui::BeginDisabled(sc.blending_mode == BlendingMode::Opaque);
                context_->mark_modified_if(ImGui::SliderInt
                    ("##rendering_layer", &sc.rendering_layer, -32, 31));
                ImGui::EndDisabled();
            });

            layout::drag_float2_control<u8"UV Offset">
            (
                sc.uv_offset, 0.0f, { 0.0f, 0.0f },
                std::nullopt, std::nullopt, u8"%.2f"
            );
        
            layout::drag_float2_control<u8"UV Scale">
            (
                sc.uv_scale, 0.0f, { 1.0f, 1.0f },
                std::nullopt, std::nullopt, u8"%.2f"
            );
        });

        check_then_draw<TextureComponent, u8"Texture">(entity, [this](Entity ent)
        {
            auto& txc{ ent.get_mut<TextureComponent>() };
            auto& am{ context_->get_owner()->get_assets_manager_mut() };

            layout::two_columns_table<u8"Asset">([&]()
            {
                String display_name{ txc.tex_uuid.empty() ? u8"<No Texture>" :
                    am.get_asset_path(txc.tex_uuid).filename().string() };

                float avail_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(avail_width);
                if(ImGui::Button(display_name.raw(), ImVec2(avail_width, 0.0f)))
                {
                    txc.tex_uuid = UUID(0);
                    context_->mark_modified();
                }

                if(ImGui::BeginDragDropTarget())
                {
                    if(const auto* payload{ ImGui::
                        AcceptDragDropPayload("CONTENT_BROWSER_ASSET_TEXTURE") })
                    {
                        AssetUUID dropped_uuid{ *reinterpret_cast<const AssetUUID*>(payload->Data) };
                        txc.tex_uuid = dropped_uuid;
                        context_->mark_modified();
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            layout::two_columns_table<u8"Filter">([&]()
            {
                constexpr const char* filt_opts[]{ "Linear", "Nearest" };
                int option{ static_cast<int>(txc.gtex_settings.filt) };
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if(ImGui::Combo("##filt", &option, filt_opts, (int)std::size(filt_opts)))
                {
                    txc.gtex_settings.filt = static_cast<GTexture::FiltFormat>(option);
                    context_->mark_modified();
                }
            });

            layout::two_columns_table<u8"Wrapping">([&]()
            {
                constexpr const char* wrap_opts[]{ "Clamp to Edge", "Repeat" };
                int option{ static_cast<int>(txc.gtex_settings.wrap) };
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                if(ImGui::Combo("##wrap", &option, wrap_opts, (int)std::size(wrap_opts)))
                {
                    txc.gtex_settings.wrap = static_cast<GTexture::WrapFormat>(option);
                    context_->mark_modified();
                }
            });

            Texture* tex{ am.get_asset<Texture>(txc.resolved_tex.handle) };
            glm::vec2 cell_size{ float(txc.cell_size.first), float(txc.cell_size.second) };
            if(layout::drag_float2_control<u8"Cell Size">(cell_size, 1.0f,
                tex ? glm::vec2(float(tex->get_width()), float(tex->get_height())) : glm::vec2(1.0f),
                glm::vec2(0.0f), glm::vec2(0.0f), u8"%.0f px"
            )) {
                txc.cell_size = { int(cell_size.x), int(cell_size.y) };
                context_->mark_modified();
            }

            glm::vec2 cell_coords{ float(txc.cell_coords.first), float(txc.cell_coords.second) };
            if(layout::drag_float2_control<u8"Cell Coords">(cell_coords, 1.0f,
                glm::vec2(0.0f, 0.0f),
                glm::vec2(0.0f), glm::vec2(0.0f), u8"%.0f"
            )) {
                txc.cell_coords = { int(cell_coords.x), int(cell_coords.y) };
                context_->mark_modified();
            }
        });

        check_then_draw<AnimatorComponent, u8"Animation">(entity, [this](Entity ent)
        {
            ImGui::SameLine();
            bool playing{ ent.is_anim_playing() };
            if(playing) {
                bool paused{ ent.is_anim_paused() };
                const char* second_text{ paused ? "Resume" : "Pause" };
                float avail_width{ ImGui::GetContentRegionAvail().x };
                float first_btn_width{ ImGui::CalcTextSize("Stop").x
                    + ImGui::GetStyle().FramePadding.x * 2.0f };
                float second_btn_width{ ImGui::CalcTextSize(second_text).x
                    + ImGui::GetStyle().FramePadding.x * 2.0f };
                float spacing{ ImGui::GetStyle().ItemSpacing.x };
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail_width
                    - first_btn_width - spacing - second_btn_width);

                if(ImGui::SmallButton("Stop")) ent.anim_stop();
                ImGui::SameLine();
                if(paused) { if(ImGui::SmallButton("Resume")) ent.anim_resume(); }
                else { if(ImGui::SmallButton("Pause")) ent.anim_pause(); }
            } else {
                float avail_width{ ImGui::GetContentRegionAvail().x };
                float btn_width{ ImGui::CalcTextSize("Play").x
                    + ImGui::GetStyle().FramePadding.x * 2.0f };
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail_width - btn_width);

                if(ImGui::SmallButton("Play")) ent.anim_play();
            }

            auto& ac{ ent.get_mut<AnimatorComponent>() };
            auto& am{ context_->get_owner()->get_assets_manager_mut() };

            layout::two_columns_table<u8"Asset">([&]()
            {
                String display_name{ ac.anim_uuid.empty() ? u8"<No Animation>" :
                    am.get_asset_path(ac.anim_uuid).filename().string() };

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if(ImGui::Button(display_name.raw(),
                    ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                {
                    ac.anim_uuid = UUID(0);
                    context_->mark_modified();
                }

                if(ImGui::BeginDragDropTarget())
                {
                    if(const auto* payload{ ImGui::
                        AcceptDragDropPayload("CONTENT_BROWSER_ASSET_ANIMATION") })
                    {
                        AssetUUID dropped_uuid{ *reinterpret_cast<const AssetUUID*>(payload->Data) };
                        ac.anim_uuid = dropped_uuid;
                        context_->mark_modified();
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            layout::two_columns_table<u8"Filter">([&]()
            {
                constexpr const char* filt_opts[]{ "Linear", "Nearest" };
                int option{ static_cast<int>(ac.gtex_settings.filt) };
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if(ImGui::Combo("##filt", &option, filt_opts, (int)std::size(filt_opts)))
                {
                    ac.gtex_settings.filt = static_cast<GTexture::FiltFormat>(option);
                    context_->mark_modified();
                }
            });

            layout::two_columns_table<u8"Wrapping">([&]()
            {
                constexpr const char* wrap_opts[]{ "Clamp to Edge", "Repeat" };
                int option{ static_cast<int>(ac.gtex_settings.wrap) };
                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                if(ImGui::Combo("##wrap", &option, wrap_opts, (int)std::size(wrap_opts)))
                {
                    ac.gtex_settings.wrap = static_cast<GTexture::WrapFormat>(option);
                    context_->mark_modified();
                }
            });

            glm::vec2 cell_size
            {
                float(ac.curr_cell_size.first ),
                float(ac.curr_cell_size.second)
            };
            layout::drag_float2_control<u8"Cell Size">
            (
                cell_size, 0.0f, glm::vec2(1.0f),
                std::nullopt, std::nullopt, u8"%.0f px"
            );
            glm::vec2 cell_coords
            {
                float(ac.curr_cell_coords.first ),
                float(ac.curr_cell_coords.second)
            };
            layout::drag_float2_control<u8"Cell Coords">
            (
                cell_coords, 0.0f, glm::vec2(0.0f),
                std::nullopt, std::nullopt, u8"%.0f"
            );

            if(ac.anim_uuid.empty()) return;
            AssetHandle anim_handle{ am.load_asset(ac.anim_uuid) };
            if(!am.is_handle_valid(anim_handle)) return;

            String start_clip{ ac.has_clip() ? String(ac.get_clip()) : String(u8"No Clip") };
            Animation* anim{ am.get_asset<Animation>(anim_handle) };
            auto& clip_names{ anim->get_clip_names() };
            layout::two_columns_table<u8"Start Clip">([&]()
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if(ImGui::BeginCombo("##clip_name", start_clip.raw()))
                {
                    bool no_clip{ !ac.has_clip() };
                    if(ImGui::Selectable("No Clip", no_clip))
                    {
                        ac.clip[0] = u8'\0';
                        context_->mark_modified();
                    }
                    if(no_clip) ImGui::SetItemDefaultFocus();
                    for(Size i{}; i < clip_names.size(); i++)
                    {
                        bool is_selected{ clip_names[i] == start_clip };
                        if(ImGui::Selectable(clip_names[i].raw(), is_selected))
                            { ac.set_clip(clip_names[i]) ;}
                        if(is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            });
            
            const auto* state{ context_->animator_state(ent.get_handle()) };
            if(!state) return;
            layout::tree_node_branch<u8"State">([&]()
            {
                const String& active{ state->active };
                layout::two_columns_table<u8"Active Clip">([&]()
                {
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::Button(active.empty() ? "<No Clip>" : active.raw(),
                        ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                    
                });
                if(anim->failed_contains(active))
                {
                    const char* text{ "Active clip invalid!" };
                    float avail_width{ ImGui::GetContentRegionAvail().x };
                    float text_width{ ImGui::CalcTextSize(text).x };
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail_width - text_width);
                    ImGui::TextColored({ 1.0f, 0.0f, 0.0f, 1.0f }, text);
                }

                float acc{ static_cast<float>(state->acc) };
                layout::drag_float_control<u8"Time Acc">
                    (acc, 0.0f, 0.0f, std::nullopt, u8"%.2f");

                float frame_idx{ static_cast<float>(state->frame_index) };
                layout::drag_float_control<u8"Frame Idx">
                    (frame_idx, 0.0f, 0.0f, std::nullopt, u8"%.0f");
            });
        });

        check_then_draw<Rigidbody2DComponent, u8"Rigidbody 2D">(entity, [this](Entity ent)
        {
            auto& rbc{ ent.get_mut<Rigidbody2DComponent>() };
            const auto& tc{ ent.get<TransformComponent>() };
            layout::two_columns_table<u8"Body Type">([&]()
            {
                constexpr const char* items[]{ "Static", "Kinematic", "Dynamic" };
                int option{ static_cast<int>(rbc.type) };

                float available_width{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(available_width);
                if(ImGui::Combo("##body_type", &option, items, (int)std::size(items)))
                {
                    rbc.type = static_cast<BodyType>(option);
                    context_->mark_modified();
                }
            });
            layout::drag_float_control<u8"Mass">(rbc.mass, 0.0f, 0.0f, std::nullopt);
            
            glm::vec2 empty_vec{};
            bool is_static{ rbc.type == BodyType::Static || !ent.is_root() }; // may modify
            layout::drag_float2_control<u8"Velocity">
            (
                !is_static ? rbc.velocity : empty_vec, 0.1f, glm::vec2(0.0f),
                !is_static ? std::optional<glm::vec2>(glm::vec2(0.0f)) : std::nullopt,
                !is_static ? std::optional<glm::vec2>(glm::vec2(0.0f)) : std::nullopt
            );
            float empty_val{};
            bool want_rotate{ !is_static && !rbc.rotation_fixed };
            layout::drag_float_control<u8"Angular Vel">
            (
                want_rotate ? rbc.angular_velocity : empty_val, 0.1f, 0.0f,
                want_rotate ? std::optional<glm::vec2>(glm::vec2(0.0f)) : std::nullopt
            );

            context_->mark_modified_if(ImGui::Checkbox("Rotation Fixed", &rbc.rotation_fixed));
        });

        check_then_draw<BoxCollider2DComponent, u8"Box Collider 2D">(entity, [this](Entity ent)
        {
            auto& bcc{ ent.get_mut<BoxCollider2DComponent>() };

            layout::two_columns_table<u8"Collider Type">([&]()
            {
                constexpr const char* items[]{ "Solid", "Sensor", "One-Way" };
                int option{ static_cast<int>(bcc.type) };

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if(ImGui::Combo("##collider_type", &option, items, (int)std::size(items)))
                {
                    bcc.type = static_cast<ColliderType>(option);
                    context_->mark_modified();
                }
            });

            layout::two_columns_table<u8"Physics Layer">([&]()
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                uint8 index{ bcc.layer_index };
                auto& physics_layers{ context_->get_owner()->get_config_mut().physics_layers };
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

            context_->mark_modified_if (
                layout::drag_float2_control<u8"Offset"> (
                    bcc.offset, 0.01f, glm::vec2(0.0f, 0.0f),
                    glm::vec2(-1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)
                ));

            context_->mark_modified_if (
                layout::drag_float2_control<u8"Scale"> (
                    bcc.size_scale, 0.01f, glm::vec2(1.0f, 1.0f),
                    glm::vec2(0.01f, 2.0f), glm::vec2(0.01f, 2.0f)
                ));

            context_->mark_modified_if (
                layout::drag_float_control<u8"Density">
                    (bcc.density, 0.01f, 1.0f, glm::vec2(0.01f, 1000.0f)));
            context_->mark_modified_if (
                layout::drag_float_control<u8"Friction">
                    (bcc.friction, 0.01f, 0.5f, glm::vec2(0.0f, 1.0f)));
            context_->mark_modified_if (
                layout::drag_float_control<u8"Restitution">
                    (bcc.restitution, 0.01f, 0.0f, glm::vec2(0.0f, 1.0f)));
        });

        check_then_draw<NativeScriptComponent, u8"Native Script">(entity, [this](Entity ent)
        {
            auto& nsc{ ent.get_mut<NativeScriptComponent>() };
            const ScriptRegistry& script_reg
                { context_->get_owner()->get_script_registry() };

            layout::two_columns_table<u8"Type">([&]()
            {
                String curr_script_name{ u8"No Script" };
                bool no_script{ nsc.script_type == script_type_null };
                if(!no_script) {
                    if(script_reg.has_script_type(nsc.script_type))
                        curr_script_name = script_reg.get_script_name(nsc.script_type);
                    else curr_script_name = String(u8"<Missing Script>");
                }
                // script_name  empty : No Script
                // script_name !empty && name  found : <Script Name>
                // script_name !empty && name !found : <Missing Script>

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if(ImGui::BeginCombo("##script", curr_script_name.raw()))
                {
                    if(ImGui::Selectable("No Script", no_script))
                    {
                        nsc.script_type = script_type_null;
                        context_->mark_modified();
                    }
                    if(no_script) ImGui::SetItemDefaultFocus();

                    for(ScriptType type : script_reg.get_script_types())
                    {
                        bool is_selected{ nsc.script_type == type };
                        if(ImGui::Selectable(std::bit_cast<const char*>(type), is_selected))
                        {
                            nsc.script_type = type;
                            context_->mark_modified();
                        }
                        if(is_selected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }
            });
            
            Script* script{ context_->script_manager_->get_script(ent.get_handle()) };
            if(script) script->on_imgui_render();
        });
    }

    void SceneHierarchyPanel::add_components_popup(Entity entity)
    {
        constexpr ImGuiPopupFlags popup_flags
        {
            ImGuiPopupFlags_MouseButtonRight |
            ImGuiPopupFlags_NoOpenOverItems
        };

        bool popup_opened{ ImGui::BeginPopupContextWindow(0, popup_flags) };
        if(!popup_opened) return;

        bool menu_opened{ ImGui::BeginMenu("Add Component") };
        if(!menu_opened){ ImGui::EndPopup(); return; }

        bool nothing_to_add{ true };
        components::each([this, &entity, &nothing_to_add](auto type_id)
        {
            using Component = decltype(type_id)::Type;
            if(entity.has<Component>()) return;
            
            if constexpr(std::is_same_v<Component, CameraComponent>)
            {
                nothing_to_add = false;
                if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                {
                    entity.emplace<CameraComponent>();
                    entity.get_mut<CameraComponent>().camera
                        .set_viewport(context_->get_viewport_h(), context_->get_viewport_w());

                    ImGui::CloseCurrentPopup();
                }
            }
            else if constexpr(std::is_same_v<Component, TextureComponent>)
            {
                if(entity.has<SpriteComponent>() && !entity.has<AnimatorComponent>())
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        entity.emplace<TextureComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else if constexpr(std::is_same_v<Component, AnimatorComponent>)
            {
                if(entity.has<SpriteComponent>() && !entity.has<TextureComponent>())
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        entity.emplace<AnimatorComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else if constexpr(std::is_same_v<Component, Rigidbody2DComponent>)
            {
                if(entity.has<SpriteComponent>())
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        entity.emplace<Rigidbody2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else if constexpr(std::is_same_v<Component, BoxCollider2DComponent>)
            {
                if(entity.has<SpriteComponent>())
                {
                    nothing_to_add = false;
                    if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                    {
                        entity.emplace<BoxCollider2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else
            {
                nothing_to_add = false;
                if(ImGui::MenuItem(type_id.name.raw_unsafe()))
                {
                    entity.emplace<Component>();
                    ImGui::CloseCurrentPopup();
                }
            }
        });
        if(nothing_to_add) { ImGui::TextColored
            (ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Nothing to add"); }

        ImGui::EndMenu();
        ImGui::EndPopup();
    }

    void SceneHierarchyPanel::general_comp_popup_content(bool& to_delete)
        { if(ImGui::MenuItem("Delete")) to_delete = true; }

    void SceneHierarchyPanel::camera_comp_popup_content(Entity entity, bool& to_delete)
    {
        if(ImGui::MenuItem("Make Master"))
            context_->set_master_camera(entity.get_handle());
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) to_delete = true;
    }

    void SceneHierarchyPanel::texture_comp_popup_content(Entity entity, bool& to_delete)
    {
        if(ImGui::MenuItem("Reload Texture"))
        {
            auto& am{ context_->get_owner()->get_assets_manager_mut() };
            am.unload_asset(entity.get<TextureComponent>().tex_uuid);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) to_delete = true;
    }

    void SceneHierarchyPanel::animator_comp_popup_content(Entity entity, bool& to_delete)
    {
        if(ImGui::MenuItem("Reload Animation"))
        {
            auto& am{ context_->get_owner()->get_assets_manager_mut() };
            am.unload_asset(entity.get<AnimatorComponent>().anim_uuid);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) to_delete = true;
    }

    void SceneHierarchyPanel::set_scene_node_selected()
    {
        is_scene_selected_ = true;
        if(context_) context_->set_selected_entity(entity_handle_null);
    }

    void SceneHierarchyPanel::set_entity_node_selected(Entity entity)
    {
        is_scene_selected_ = false;
        if(context_) context_->set_selected_entity(entity.get_handle());
    }
}
