module;
module AnimationEditorPanel;
/*
namespace {
    using namespace rke;

    static const rke::GTextureSettings s_sheet_settings
    {
        rke::GTexture::FiltFormat::Nearest,
        rke::GTexture::WrapFormat::Clamp2Edge,
        false // ui sampling: srgb has to be false
    };
}

namespace rke
{
    AnimationClipEditorPanel::AnimationClipEditorPanel(String name)
        : Panel(std::move(name))
    {
        clip_name_buffer_.fill(0);
    }

    AnimationClipEditorPanel::~AnimationClipEditorPanel() = default;

    Project* AnimationClipEditorPanel::project()
    {
        Project* proj{ app().get_project() };
        if(!proj) CORE_ERROR(u8"AnimationClipEditorPanel: No project loaded!");
        return proj;
    }

    AssetsManager& AnimationClipEditorPanel::assets_manager()
    {
        Project* proj{ app().get_project() };
        CORE_ASSERT(proj, u8"AnimationClipEditorPanel: No project loaded!");
        return proj->get_assets_manager_mut();
    }

    void AnimationClipEditorPanel::open(UUID clip_set_uuid)
    {
        editing_ = false;
        working_copy_.reset();
        if(clip_set_uuid.empty()) return;

        Project* proj{ app().get_project() };
        if(!proj) return;

        AssetsManager& manager{ proj->get_assets_manager_mut() };
        AssetHandle handle{ manager.load_asset(clip_set_uuid) };
        if(!manager.is_handle_valid(handle)) return;
        const Animation* clip_set
            { manager.get_asset<Animation>(handle) };
        if(!clip_set) return;

        editing_uuid_ = clip_set_uuid;
        editing_ = true;
        show();
        selected_clip_ = -1;
        selected_frame_ = -1;
        preview_playing_ = false;
        preview_time_ = 0.0f;
        preview_clip_ = -1;
        (void)reload_asset_context();
    }

    bool AnimationClipEditorPanel::reload_asset_context()
    {
        if(!editing_ || editing_uuid_.empty()) return false;

        AssetsManager& manager{ assets_manager() };
        AssetHandle handle{ manager.load_asset(editing_uuid_) };
        if(!manager.is_handle_valid(handle)) return false;
        const Animation* clip_set
            { manager.get_asset<Animation>(handle) };
        if(!clip_set) return false;

        working_copy_ = create_scope<Animation>(*clip_set);
        if(selected_clip_ >= static_cast<int32>(working_copy_->clips().size()))
            selected_clip_ = working_copy_->clips().empty() ? -1 : 0;
        return true;
    }

    void AnimationClipEditorPanel::save()
    {
        if(!editing_ || !working_copy_) return;

        AssetsManager& manager{ assets_manager() };
        const Path& asset_path{ manager.get_asset_path(editing_uuid_) };
        if(asset_path.empty() || !asset_path.exists()) {
            CORE_ERROR(u8"AnimationClipEditorPanel: Asset path missing for '{}'!",
                editing_uuid_.value());
            return;
        }
        if(!working_copy_->save_to(asset_path)) return;

        manager.reload_asset(editing_uuid_);
        (void)reload_asset_context();
        CORE_INFO(u8"AnimationClipEditorPanel: Saved '{}'.", asset_path);
    }

    void AnimationClipEditorPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        if(!app().get_project())
        {
            ImGui::TextWrapped("Load a project to edit animation clip assets.");
            ImGui::End();
            return;
        }
        if(!editing_ || !working_copy_)
        {
            ImGui::TextWrapped("Open an .rkanim asset to start editing.");
            ImGui::End();
            return;
        }

        draw_header();
        ImGui::Separator();

        ImGui::BeginChild("AnimEditorBody", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2));
        draw_settings_and_clip_list();
        ImGui::SameLine();
        draw_clip_params();
        ImGui::SameLine();
        draw_sheet_and_frames();
        ImGui::EndChild();

        ImGui::Separator();
        draw_transport();

        ImGui::End();
    }

    void AnimationClipEditorPanel::draw_header()
    {
        AssetsManager& manager{ assets_manager() };
        const Path& asset_path{ manager.get_asset_path(editing_uuid_) };
        ImGui::TextUnformatted("Clip Set:");
        ImGui::SameLine();
        if(!asset_path.empty())
            ImGui::TextUnformatted(asset_path.string().raw());
        else ImGui::Text("<?>");

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120.0f);
        if(ImGui::Button("Save", ImVec2(56, 0))) save();
        ImGui::SameLine();
        if(ImGui::Button("Close", ImVec2(56, 0))) close_editor();
    }

    void AnimationClipEditorPanel::draw_settings_and_clip_list()
    {
        ImGui::BeginChild("AnimSettings", ImVec2(280, 0), ImGuiChildFlags_Borders);

    // --- sheet settings ---
        layout::tree_node_branch<u8"Sheet">([&]()
        {
        // texture drop
            UUID tex{ working_copy_->texture_uuid() };
            AssetsManager& manager{ assets_manager() };
            String display{ tex.empty() ? u8"<drop texture>" :
                Path(manager.get_asset_path(tex)).filename().string() };

            ImGui::Text("Texture");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if(ImGui::Button(display.raw(), ImVec2(-1, 0))) {}
            if(ImGui::BeginDragDropTarget())
            {
                if(const auto* payload{ ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET") })
                {
                    UUID dropped{ *reinterpret_cast<const UUID*>(payload->Data) };
                    AssetHandle h{ manager.load_asset(dropped) };
                    if(manager.is_handle_valid(h) && manager.get_asset<Texture>(h))
                    {
                        working_copy_->set_texture_uuid(dropped);
                        hovered_cell_ = { -1, -1 };
                    }
                }
                ImGui::EndDragDropTarget();
            }

        // cell size (pixels) via two drag ints
            int cell[2]{ working_copy_->cell_size().first,
                         working_copy_->cell_size().second };
            ImGui::Text("Cell Size");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if(ImGui::DragInt2("##cell_size", cell, 1.0f, 1, 4096))
            {
                working_copy_->set_cell_size({ std::max(cell[0], 1),
                                               std::max(cell[1], 1) });
            }
        });

        ImGui::Spacing();

    // --- clip list ---
        layout::tree_node_branch<u8"Clips">([&]()
        {
            auto& clips{ working_copy_->clips() };
            const float list_h{ ImGui::GetContentRegionAvail().y - 24.0f };
            ImGui::BeginChild("ClipList", ImVec2(0, list_h), ImGuiChildFlags_Borders);
            for(Size i{}; i < clips.size(); i++)
            {
                const bool selected{ selected_clip_ == static_cast<int32>(i) };
                char buf[128]{};
                const Size n{ std::min<Size>(clips[i].name.length(), 127) };
                std::memcpy(buf, clips[i].name.data(), n);
                if(ImGui::Selectable(buf, selected))
                {
                    selected_clip_ = static_cast<int32>(i);
                    selected_frame_ = -1;
                }
            }
            ImGui::EndChild();

            if(ImGui::Button("+ Add", ImVec2(-1, 0)))
            {
                auto& clips{ working_copy_->clips() };
                const Size target{ clips.size() }; // index the new clip will get

                String name{ u8"Clip" };
                Size counter{ 1 };
                auto taken{ [&](const String& candidate) -> bool
                {
                    Size found{ working_copy_->find_clip(candidate) };
                    return found < clips.size() && found != target;
                } };
                while(taken(name))
                    name = String::format(u8"Clip {}", counter++);

                Size idx{ working_copy_->add_clip(name) };
                selected_clip_ = static_cast<int32>(idx);
                selected_frame_ = -1;
            }
        });

        ImGui::EndChild();
    }

    void AnimationClipEditorPanel::draw_clip_params()
    {
        ImGui::BeginChild("AnimClipParams", ImVec2(230, 0), ImGuiChildFlags_Borders);

        if(selected_clip_ < 0
        || selected_clip_ >= static_cast<int32>(working_copy_->clips().size()))
        {
            ImGui::TextDisabled("Select a clip on the left.");
            ImGui::EndChild();
            return;
        }

        const int32 index{ selected_clip_ };
        const auto& clip{ working_copy_->clips()[static_cast<Size>(index)] };

        // clip name (edit buffer, kept stable while the field is focused)
        if(!clip_name_editing_ || clip_name_edited_clip_ != index)
        {
            clip_name_buffer_.fill(0);
            const Size n{ std::min<Size>(clip.name.length(),
                clip_name_buffer_.size() - 1) };
            std::memcpy(clip_name_buffer_.data(), clip.name.data(), n);
            clip_name_edited_clip_ = index;
        }
        ImGui::Text("Name");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##clip_name", clip_name_buffer_.data(),
            clip_name_buffer_.size());
        clip_name_editing_ = ImGui::IsItemActive();
        if(!clip_name_editing_ && ImGui::IsItemDeactivatedAfterEdit())
        {
            String new_name{ str::to_char8(clip_name_buffer_.data()) };
            if(!new_name.empty())
                working_copy_->set_clip_name(index, new_name);
        }

        float fps{ clip.fps };
        if(layout::drag_float_control<u8"FPS">(fps, 0.5f, 12.0f,
            glm::vec2(0.0f, 240.0f)))
            working_copy_->set_clip_fps(index, fps);

        bool loop{ clip.loop };
        if(ImGui::Checkbox("Loop", &loop))
            working_copy_->set_clip_loop(index, loop);

        ImGui::Text("Frame Count: %d", static_cast<int>(clip.frames.size()));
        if(!clip.frames.empty() && selected_frame_ >= 0)
        {
            const auto& cell{ clip.frames[static_cast<Size>(selected_frame_)] };
            ImGui::Text("Selected: [%d, %d]", cell.first, cell.second);
        }

        ImGui::Spacing();

        // delete / clear
        if(ImGui::Button("Delete Clip", ImVec2(-1, 0)))
        {
            working_copy_->remove_clip(index);
            selected_clip_ = working_copy_->clips().empty() ? -1 : 0;
            selected_frame_ = -1;
        }
        if(selected_frame_ >= 0
        && selected_frame_ < static_cast<int32>(clip.frames.size()))
        {
            if(ImGui::Button("Remove Frame", ImVec2(-1, 0)))
            {
                working_copy_->remove_frame(index, static_cast<Size>(selected_frame_));
                selected_frame_ = -1;
            }
        }

        ImGui::EndChild();
    }

    void AnimationClipEditorPanel::draw_sheet_and_frames()
    {
        ImGui::BeginChild("AnimSheet", ImVec2(0, 0), ImGuiChildFlags_Borders);

        Texture* tex{ resolve_sheet_texture() };
        if(!tex)
        {
            ImGui::TextDisabled("Drop a sheet texture on the left panel.");
            ImGui::EndChild();
            return;
        }
        GTexture* gtex{ resolve_sheet_gtex() };
        if(!gtex)
        {
            ImGui::TextDisabled("Failed to upload sheet texture.");
            ImGui::EndChild();
            return;
        }

        const float tex_w{ static_cast<float>(tex->get_width()) };
        const float tex_h{ static_cast<float>(tex->get_height()) };
        const std::pair<int, int> cell_size{ working_copy_->cell_size() };
        const int cols{ std::max(1, static_cast<int>(tex_w)
            / std::max(1, cell_size.first)) };
        const int rows{ std::max(1, static_cast<int>(tex_h)
            / std::max(1, cell_size.second)) };

        sheet_uv_scale_ = { float(cell_size.first) / tex_w,
                            float(cell_size.second) / tex_h };

        // draw sheet (flipped so row 0 appears on top)
        const float avail{ ImGui::GetContentRegionAvail().x };
        ImGui::Image(static_cast<ImTextureID>(gtex->get_gal_id()),
            ImVec2(avail, avail * tex_h / tex_w),
            ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

        // grid + cell hover picking
        // NOTE: the sheet is drawn flipped ({0,1}->{1,0}) so the top image row
        // appears first, matching ContentBrowser. The stored frame cell
        // coordinates are interpreted by the renderer exactly as the manual
        // Sprite cell_coords workflow does; if a produced clip looks vertically
        // mirrored in-game, flip the rows when authoring (mirror of the whole
        // sheet), not by swapping cells.
        if(ImGui::IsItemHovered())
        {
            const glm::vec2 mouse{ ImGui::GetMousePos().x - ImGui::GetItemRectMin().x,
                                   ImGui::GetMousePos().y - ImGui::GetItemRectMin().y };
            const glm::vec2 item{ ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x,
                                  ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y };
            if(item.x > 0.0f && item.y > 0.0f)
            {
                int c{ static_cast<int>(mouse.x / item.x * float(cols)) };
                int r{ static_cast<int>(mouse.y / item.y * float(rows)) };
                c = std::clamp(c, 0, cols - 1);
                r = std::clamp(r, 0, rows - 1);
                hovered_cell_ = { c, r };
                if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && selected_clip_ >= 0)
                {
                    working_copy_->add_frame(selected_clip_, hovered_cell_);
                    selected_frame_ = static_cast<int32>(
                        working_copy_->clips()
                            [static_cast<Size>(selected_clip_)].frames.size()) - 1;
                }
            }
        }

        const ImVec2 min{ ImGui::GetItemRectMin() };
        const ImVec2 max{ ImGui::GetItemRectMax() };
        const glm::vec2 item_size{ max.x - min.x, max.y - min.y };
        ImDrawList* draw{ ImGui::GetWindowDrawList() };

        // grid lines
        for(int c{ 1 }; c < cols; c++)
        {
            float x{ min.x + item_size.x * float(c) / float(cols) };
            draw->AddLine(ImVec2(x, min.y), ImVec2(x, max.y),
                IM_COL32(255, 255, 255, 40));
        }
        for(int r{ 1 }; r < rows; r++)
        {
            float y{ min.y + item_size.y * float(r) / float(rows) };
            draw->AddLine(ImVec2(min.x, y), ImVec2(max.x, y),
                IM_COL32(255, 255, 255, 40));
        }

        // hover highlight
        if(hovered_cell_.first >= 0)
        {
            float x0{ min.x + item_size.x * hovered_cell_.first / float(cols) };
            float y0{ min.y + item_size.y * hovered_cell_.second / float(rows) };
            float x1{ x0 + item_size.x / float(cols) };
            float y1{ y0 + item_size.y / float(rows) };
            draw->AddRect(ImVec2(x0, y0), ImVec2(x1, y1),
                IM_COL32(120, 200, 255, 160), 0.0f, 0, 2.0f);
        }

        // preview current frame highlight is drawn in the transport row;
        // here we only mark hover + append new frames on click

        ImGui::Spacing();

        // selected clip's frames as a strip of small cell previews
        if(selected_clip_ >= 0
        && selected_clip_ < static_cast<int32>(working_copy_->clips().size()))
        {
            const auto& clip{ working_copy_->clips()[static_cast<Size>(selected_clip_)] };
            ImGui::Text("Frames (%d) - click a cell to append",
                static_cast<int>(clip.frames.size()));
            if(clip.frames.empty())
            {
                ImGui::TextDisabled("No frames yet.");
            }
            else
            {
                const float frame_avail{ ImGui::GetContentRegionAvail().x };
                const float cell_w{ 48.0f };
                const int per_row{ std::max(1, static_cast<int>(frame_avail / (cell_w + 4.0f))) };
                for(Size i{}; i < clip.frames.size(); i++)
                {
                    if(i > 0 && i % static_cast<Size>(per_row) != 0)
                        ImGui::SameLine();
                    const auto& frame_cell{ clip.frames[i] };
                    const glm::vec2 uv_min{ float(frame_cell.first) * sheet_uv_scale_.x,
                        float(frame_cell.second) * sheet_uv_scale_.y };
                    const glm::vec2 uv_max{ uv_min + sheet_uv_scale_ };
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    const bool selected{ selected_frame_ == static_cast<int32>(i) };
                    if(selected)
                        ImGui::PushStyleColor(ImGuiCol_Border,
                            ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                    if(ImGui::ImageButton("##frame_thumb",
                        ImTextureRef(static_cast<ImTextureID>(
                            static_cast<uint64>(gtex->get_gal_id()))),
                        ImVec2(cell_w, cell_w),
                        ImVec2(uv_min.x, uv_max.y), ImVec2(uv_max.x, uv_min.y)))
                    {
                        selected_frame_ = static_cast<int32>(i);
                    }
                    if(selected)
                        ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                    if(ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("[%d, %d]  frame %d",
                            frame_cell.first, frame_cell.second,
                            static_cast<int>(i));
                    }
                    ImGui::PopID();
                }
            }
        }

        ImGui::EndChild();
    }

    void AnimationClipEditorPanel::draw_transport()
    {
        auto& clips{ working_copy_->clips() };
        if(clips.empty())
        {
            ImGui::TextDisabled("No clips to preview.");
            return;
        }

        // clip selector for transport
        int32 transport_clip{ preview_clip_ >= 0
            && preview_clip_ < static_cast<int32>(clips.size())
                ? preview_clip_ : 0 };
        if(ImGui::BeginCombo("Clip", clips[static_cast<Size>(transport_clip)].name.raw()))
        {
            for(Size i{}; i < clips.size(); i++)
            {
                const bool is_sel{ static_cast<int32>(i) == transport_clip };
                if(ImGui::Selectable(clips[i].name.raw(), is_sel))
                {
                    transport_clip = static_cast<int32>(i);
                    preview_playing_ = false;
                    preview_time_ = 0.0f;
                }
                if(is_sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        preview_clip_ = transport_clip;

        ImGui::SameLine();
        if(preview_playing_)
        {
            if(ImGui::Button("Pause")) preview_playing_ = false;
        }
        else
        {
            if(ImGui::Button("Play")) { preview_playing_ = true; preview_time_ = 0.0f; }
        }
        ImGui::SameLine();
        if(ImGui::Button("Stop"))
        {
            preview_playing_ = false;
            preview_time_ = 0.0f;
            selected_frame_ = -1;
        }
        ImGui::SameLine();
        ImGui::Text("Frame: %d", selected_frame_ >= 0 ? selected_frame_ + 1 : 0);

        // advance preview
        if(preview_playing_ && preview_clip_ >= 0
        && preview_clip_ < static_cast<int32>(clips.size()))
        {
            const auto& clip{ clips[static_cast<Size>(preview_clip_)] };
            const Size count{ clip.frames.size() };
            if(count == 0) { preview_playing_ = false; return; }

            preview_time_ += ImGui::GetIO().DeltaTime;
            const float fps{ clip.fps > 0.0f ? clip.fps : 1.0f };
            float t{ preview_time_ * fps };
            int32 frame{ static_cast<int32>(t) };
            if(frame >= static_cast<int32>(count))
            {
                if(clip.loop)
                {
                    frame = frame % static_cast<int32>(count);
                    preview_time_ = float(frame) / fps;
                }
                else
                {
                    preview_playing_ = false;
                    frame = static_cast<int32>(count) - 1;
                }
            }
            selected_frame_ = frame;
        }
    }

    Texture* AnimationClipEditorPanel::resolve_sheet_texture()
    {
        AssetsManager& manager{ assets_manager() };
        UUID tex{ working_copy_->texture_uuid() };
        if(tex.empty()) return nullptr;
        AssetHandle h{ manager.load_asset(tex) };
        if(!manager.is_handle_valid(h)) return nullptr;
        return manager.get_asset<Texture>(h);
    }

    GTexture* AnimationClipEditorPanel::resolve_sheet_gtex()
    {
        Texture* tex{ resolve_sheet_texture() };
        if(!tex) return nullptr;
        return tex->get_gtexture(s_sheet_settings);
    }
}
*/
