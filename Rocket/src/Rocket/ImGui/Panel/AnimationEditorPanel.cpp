module;

#include <imgui_internal.h>

module AnimationEditorPanel;

import Log;
import String;
import Types;
import Path;
import Animation;
import Texture;
import GTexture;
import Project;
import Application;
import ImGuiSetup;
import Components;
import Layout;

namespace {
    static void ImGui_ImplOpenGL3_DisableBindSampler(const ImDrawList*, const ImDrawCmd*)
        { rke::imgui::disable_bind_sampler(); }
}

namespace rke
{
    AnimationEditorPanel::AnimationEditorPanel(String name)
        : Panel(std::move(name)) { clear(); }

    AnimationEditorPanel::~AnimationEditorPanel() { clear(); }

    void AnimationEditorPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        AssetUUID dropped_uuid{ 0 };
        bool has_drop{ false };
        {
            ImGuiWindow* window{ ImGui::GetCurrentWindow() };
            if(ImGui::BeginDragDropTargetCustom(window->Rect(), ImGui::GetID("##anim_drop")))
            {
                if(const ImGuiPayload* payload{ ImGui::
                    AcceptDragDropPayload("CONTENT_BROWSER_ASSET_ANIMATION") })
                {
                    dropped_uuid = *reinterpret_cast<const AssetUUID*>(payload->Data);
                    has_drop = true;
                }
                ImGui::EndDragDropTarget();
            }
        }

        Project* project{ app().get_project() };
        if(!project)
        {
            ImGui::TextDisabled("No project loaded.");
            ImGui::End(); 
            return;
        }

        if(has_drop) open(dropped_uuid);

        if(ImGui::SmallButton("~")) clear();
        ImGui::SameLine();

        if(asset_uuid_.empty())
        {
            ImGui::TextDisabled("No animation.");
            ImGui::End();
            return;
        }

        CORE_ASSERT(!asset_path_.empty(), u8"AnimationEditorPanel:"
            u8" Asset '{}' path empty!", asset_uuid_.value())
        ImGui::Text("%s", asset_path_.string().raw());
        if(modified_) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "* unsaved");
        }

        auto& am{ project->get_assets_manager_mut() };
        anim_popup(am);

        ImGui::BeginChild("##clips_managing", ImVec2(160.0f, 0.0f), ImGuiChildFlags_Borders);
        clips_managing(am);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##clip", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        selected_clip_editing(am);
        ImGui::EndChild();

        ImGui::End();
    }

    void AnimationEditorPanel::clear()
    {
        asset_uuid_ = UUID(0);
        asset_path_.clear();
        anim_ = Animation{};
        modified_ = false;

        selected_clip_.clear();
        selected_frame_ = 0;
        preview_playing_ = false;
        preview_time_ = 0.0f;
    }

    void AnimationEditorPanel::open(AssetUUID uuid)
    {
        clear();
        if(uuid.empty()) return;

        Project* project{ app().get_project() };
        if(!project) return;

        AssetsManager& am{ project->get_assets_manager_mut() };
        Animation* asset{ am.get_asset<Animation>(am.load_asset(uuid)) };
        if(!asset)
        {
            CORE_ERROR(u8"AnimationEditorPanel: Failed to load animation '{}'!", uuid.value());
            return;
        }
    // the panel edits a copy, the asset manager keeps holding the on-disk one
        asset_uuid_ = uuid;
        asset_path_ = am.get_asset_path(asset_uuid_);
        anim_ = *asset;
    }

    void AnimationEditorPanel::clips_managing(AssetsManager& am)
    {
        using namespace literals;
    // texture slot
        const AssetUUID tex_uuid{ anim_.get_tex_uuid() };
        const String tex_name{ tex_uuid.empty()
            ? u8"<No Texture>"_s
            : am.get_asset_path(tex_uuid).filename().string() };

        float avail_width{ ImGui::GetContentRegionAvail().x };
        ImGui::SetNextItemWidth(avail_width);
        ImGui::Button(tex_name.raw(), ImVec2(avail_width, 0.0f));
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Click to clear.");

        if(ImGui::IsItemClicked() && !tex_uuid.empty())
        {
            anim_.tex_uuid_ = UUID(0);
            modified_ = true;
        }

        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* payload{ ImGui::
                AcceptDragDropPayload("CONTENT_BROWSER_ASSET_TEXTURE") })
            {
                anim_.tex_uuid_ = *reinterpret_cast<const AssetUUID*>(payload->Data);
                anim_.resolved_tex_ = AssetResolve{}; // the old handle is stale
                modified_ = true;
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::Separator();

        auto set_selected{ [this](const String& name)
        {
            selected_clip_ = name;
            selected_frame_ = 0;
            preview_time_ = 0.0f;
        }};

    // clips managing
        std::vector<const String*> to_remove{};
        for(const String& name : anim_.get_clip_names())
        {
            if(ImGui::Selectable(name.raw(), name == selected_clip_))
                set_selected(name);
            if(clip_popup(name)) to_remove.push_back(&name);
        }
        for(const String* name : to_remove)
        {
            if(*name == selected_clip_) selected_clip_.clear();
            anim_.remove_clip(*name);
            modified_ = true;
        }
        to_remove.clear();
        if(anim_.get_clip_names().empty()) ImGui::TextDisabled("no clips");

        ImGui::Separator();

        ImGui::SetNextItemWidth(avail_width);
        if(ImGui::Button("Add Clip", ImVec2(avail_width, 0.0f)))
        {
        // first free name: "Clip", "Clip 2", "Clip 3", ...
            auto& names{ anim_.get_clip_names() };
            String name{ u8"Clip"_s };
            for(uint32 suffix{ 2 };
                std::find(names.begin(), names.end(), name) != names.end();
                ++suffix
            ) name = String::format(u8"Clip {}", suffix);

            anim_.emplace_clip(name, AnimClip{});
            modified_ = true;
            set_selected(name);
        }
    }

    void AnimationEditorPanel::selected_clip_editing(AssetsManager& am) // TO MODIFY
    {
        using namespace literals;

        const AnimClip* source{ selected_clip_.empty()
            ? nullptr : anim_.get_clip(selected_clip_) };
        if(!source) {
            ImGui::TextDisabled("Select a clip on the left, or create one.");
            return;
        }

        AnimClip clip{ *source };
        bool changed{ false };
        constexpr float basic_width{ 160.0f };

    // name editing
        char name_buffer[AnimatorComponent::clip_name_cap]{};
        std::memcpy(name_buffer, selected_clip_.raw(), sizeof(name_buffer) - 1);
        ImGui::SetNextItemWidth(basic_width + ImGui::CalcTextSize("Cell Size").x);
        if(ImGui::InputText("##clip_name", name_buffer, sizeof(name_buffer),
            ImGuiInputTextFlags_EnterReturnsTrue))
        {
            String new_name{ String{ str::to_char8(name_buffer) } };
            const std::vector<String>& names{ anim_.get_clip_names() };
            const bool taken{ std::find(names.begin(), names.end(), new_name) != names.end() };

            if(!new_name.empty() && !taken && new_name != selected_clip_)
            {
                const String old_name{ selected_clip_ };
                anim_.emplace_clip(new_name, clip);

            // whoever pointed at the old name follows the clip to its new one
                for(const String& other : names)
                {
                    if(other == new_name || other == old_name) continue;
                    const AnimClip* other_clip{ anim_.get_clip(other) };
                    if(!other_clip || other_clip->next != old_name) continue;

                    AnimClip patched{ *other_clip };
                    patched.next = new_name;
                    anim_.replace_clip(other, patched);
                }

                anim_.remove_clip(old_name);
                selected_clip_ = std::move(new_name);
                changed = true;
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(enter to rename)");

    // clip parameters
        const float row_height{ ImGui::GetFrameHeight() };
        int cell[2]{ clip.cell_size.first, clip.cell_size.second };
        ImGui::SetNextItemWidth(basic_width);
        if(ImGui::DragInt2("Cell Size", cell, 1.0f, 1, 8192, "%d px"))
        { 
            clip.cell_size = { cell[0], cell[1] };
            changed = true;
        }

        ImGui::SameLine();
        layout::vertical_separator(row_height);

        int fps{ static_cast<int>(clip.fps) };
        ImGui::SetNextItemWidth(basic_width * 0.4f);
        if(ImGui::DragInt("FPS", &fps, 1.0f, 1, 240))
        {
            clip.fps = static_cast<uint32>(fps);
            changed = true;
        }

        ImGui::SameLine();
        layout::vertical_separator(row_height);

        if(ImGui::Checkbox("Loop", &clip.loop)) changed = true;

        const String next_label{ clip.next.empty() ? u8"<None>"_s : clip.next };
        ImGui::SetNextItemWidth(basic_width);
        if(ImGui::BeginCombo("Next", next_label.raw()))
        {
            if(ImGui::Selectable("<None>", clip.next.empty()))
                { clip.next.clear(); changed = true; }

            for(const String& other : anim_.get_clip_names())
            {
                if(other == selected_clip_) continue; // no self transition
                if(ImGui::Selectable(other.raw(), other == clip.next))
                    { clip.next = other; changed = true; }
            }
            ImGui::EndCombo();
        }

    // frames
        ImGui::Separator();
        ImGui::Text("Frames: %zu", static_cast<size_t>(clip.frames.size()));

        Size edit_index{ clip.frames.size() };
        int  edit_action{ 0 }; // 1 remove, 2 move left, 3 move right
        for(Size i{}; i < clip.frames.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            const String label{ String::format(u8"{}:({},{})", i,
                clip.frames[i].first, clip.frames[i].second) };

            if(ImGui::Selectable(label.raw(), i == selected_frame_,
                ImGuiSelectableFlags_None, ImVec2(84.0f, 0.0f)))
                selected_frame_ = i;

            if(ImGui::BeginPopupContextItem("##frame"))
            {
                if(ImGui::MenuItem("Remove")) { edit_index = i; edit_action = 1; }
                if(ImGui::MenuItem("Move Left", nullptr, false, i > 0))
                { edit_index = i; edit_action = 2; }
                if(ImGui::MenuItem("Move Right", nullptr, false, i + 1 < clip.frames.size()))
                { edit_index = i; edit_action = 3; }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            if((i + 1) % 6 != 0 && (i + 1) < clip.frames.size()) ImGui::SameLine();
        }

    // applied after the loop, the indices in it are only valid before an edit
        if(edit_action != 0 && edit_index < clip.frames.size())
        {
            const auto pos{ clip.frames.begin() + static_cast<std::ptrdiff_t>(edit_index) };
            switch(edit_action)
            {
            case 1: clip.frames.erase(pos); break;
            case 2: std::swap(*pos, *(pos - 1)); --selected_frame_; break;
            case 3: std::swap(*pos, *(pos + 1)); ++selected_frame_; break;
            default: break;
            }
            if(selected_frame_ >= clip.frames.size())
                selected_frame_ = clip.frames.empty() ? 0 : clip.frames.size() - 1;
            changed = true;
        }

        if(ImGui::Button("+ Frame"))
        {
        // the next cell of the same row, the usual way sheets are laid out;
        // clicking the sheet below overrides it anyway
            std::pair<int, int> cell{ 0, 0 };
            if(!clip.frames.empty())
            { cell = clip.frames.back(); ++cell.first; }

            clip.frames.push_back(cell);
            selected_frame_ = clip.frames.size() - 1;
            changed = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Clear Frames") && !clip.frames.empty())
        {
            clip.frames.clear();
            selected_frame_ = 0;
            changed = true;
        }

    // preview playback
        if(clip.frames.empty()) preview_playing_ = false;
        else {
            if(selected_frame_ >= clip.frames.size())
                selected_frame_ = clip.frames.size() - 1;

            if(preview_playing_)
            {
                preview_time_ += ImGui::GetIO().DeltaTime
                    * static_cast<float>(std::max(1u, clip.fps));

                const Size total{ clip.frames.size() };
                const Size at{ static_cast<Size>(preview_time_) };

                if(clip.loop) selected_frame_ = at % total;
                else if(at + 1 >= total) preview_playing_ = false;
                else selected_frame_ = at;
            }
        }

        if(ImGui::Button(preview_playing_ ? "Pause" : "Play"))
        {
            preview_playing_ = !preview_playing_;
            preview_time_ = static_cast<float>(selected_frame_);
        }
        ImGui::SameLine();
        ImGui::Text("frame %zu / %zu", static_cast<size_t>(clip.frames.empty()
            ? 0 : selected_frame_ + 1), static_cast<size_t>(clip.frames.size()));

        ImGui::Separator();

    // sheet, click a cell to select the frame using it or to append one   
        const AssetHandle tex_handle{ anim_.get_tex_handle(am).first };
        Texture* texture{ am.get_asset<Texture>(tex_handle) };
        GTexture* gtex{ texture ?
            texture->get_gtexture(GTextureSettings
            {
                .filt = GTexture::FiltFormat::Nearest,
                .wrap = GTexture::WrapFormat::Clamp2Edge,
                .srgb = false
            }) : nullptr };
        CORE_ASSERT(gtex, u8"AnimationEditorPanel: Texture format invalid!");

        const float tex_w{ static_cast<float>(texture->get_width()) };
        const float tex_h{ static_cast<float>(texture->get_height()) };
        const float cell_w_px{ static_cast<float>(std::max(1, clip.cell_size.first)) };
        const float cell_h_px{ static_cast<float>(std::max(1, clip.cell_size.second)) };
        const int columns{ std::max(1, static_cast<int>(tex_w / cell_w_px)) };
        const int rows   { std::max(1, static_cast<int>(tex_h / cell_h_px)) };

        float scale{ 1.0f };
        if(const float avail{ ImGui::GetContentRegionAvail().x };
            avail > 16.0f && tex_w > 0.0f)
            scale = std::clamp(avail / tex_w, 0.05f, 8.0f);

        const ImVec2 sheet_size{ tex_w * scale, tex_h * scale };
        const ImVec2 origin{ ImGui::GetCursorScreenPos() };
        const float  cell_w{ cell_w_px * scale };
        const float  cell_h{ cell_h_px * scale };

    // uv0/uv1 below already flip the sheet upright, so cell row 0 is the one
    // drawn at the bottom. Drop the 'rows - 1 -' flips if your highlight ends
    // up mirrored.
        const auto cell_rect{ [&](const std::pair<int, int>& cell)
        {
            const float x{ origin.x + static_cast<float>(cell.first) * cell_w };
            const float y{ origin.y + static_cast<float>(rows - 1 - cell.second) * cell_h };
            return std::pair<ImVec2, ImVec2>{ { x, y }, { x + cell_w, y + cell_h } };
        }};

        ImGui::GetWindowDrawList()->AddCallback
            (ImGui_ImplOpenGL3_DisableBindSampler, nullptr);
        ImGui::Image(ImTextureRef(static_cast<ImTextureID>(gtex->get_gal_id())),
            sheet_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        ImGui::GetWindowDrawList()->AddCallback
            (ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);

        ImGui::SetCursorScreenPos(origin);
        ImGui::InvisibleButton("##sheet", sheet_size, ImGuiButtonFlags_MouseButtonLeft);

        ImDrawList* draw{ ImGui::GetWindowDrawList() };
        for(int column{}; column <= columns; ++column)
        {
            const float x{ origin.x + static_cast<float>(column) * cell_w };
            draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + sheet_size.y),
                IM_COL32(255, 255, 255, 40));
        }
        for(int row{}; row <= rows; ++row)
        {
            const float y{ origin.y + static_cast<float>(row) * cell_h };
            draw->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + sheet_size.x, y),
                IM_COL32(255, 255, 255, 40));
        }

        for(Size i{}; i < clip.frames.size(); ++i)
        {
            const auto [lo, hi]{ cell_rect(clip.frames[i]) };
            const bool current{ i == selected_frame_ };
            draw->AddRect(lo, hi, current ? IM_COL32(255, 210, 0, 255)
                : IM_COL32(120, 190, 255, 200), 0.0f, 0, current ? 2.5f : 1.5f);
        }

        if(ImGui::IsItemHovered())
        {
            const ImVec2 mouse{ ImGui::GetIO().MousePos };
            const int column{ static_cast<int>((mouse.x - origin.x) / cell_w) };
            const int row{ rows - 1 - static_cast<int>((mouse.y - origin.y) / cell_h) };

            if(column >= 0 && column < columns && row >= 0 && row < rows)
            {
                const auto [lo, hi]{ cell_rect({ column, row }) };
                draw->AddRectFilled(lo, hi, IM_COL32(255, 255, 255, 40));
                ImGui::SetTooltip("(%d, %d)", column, row);

                if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    Size found{ clip.frames.size() }; // append when unused
                    for(Size i{}; i < clip.frames.size(); ++i)
                        if(clip.frames[i] == std::pair<int, int>{ column, row })
                        { found = i; break; }

                    if(found == clip.frames.size())
                    {
                        clip.frames.emplace_back(column, row);
                        found = clip.frames.size() - 1;
                        changed = true;
                    }
                    selected_frame_ = found;
                }
            }
        }

        if(changed)
        {
            anim_.replace_clip(selected_clip_, clip);
            modified_ = true;
        }
    }

    void AnimationEditorPanel::anim_popup(AssetsManager& am)
    {
        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow)
        && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup("##anim_popup");

        if(!ImGui::BeginPopup("##anim_popup")) return;

        if(ImGui::MenuItem("Save"))
        {
            if(anim_.save_to(asset_path_))
            {
                modified_ = false;
                am.unload_asset(asset_uuid_);
                CORE_INFO(u8"AnimationEditorPanel: Saved '{}'.", asset_path_);
            }
            else CORE_ERROR(u8"AnimationEditorPanel: Failed to save '{}'!", asset_path_);
        }

        if(ImGui::MenuItem("Reload"))
        {
            am.unload_asset(asset_uuid_);
            open(asset_uuid_);
        }

        ImGui::EndPopup();
    }

    bool AnimationEditorPanel::clip_popup(const String& name)
    {
        if(name.empty()) return false;
        if(!ImGui::BeginPopupContextItem(name.raw())) return false;

        bool to_remove{ false };
        if(ImGui::MenuItem("Remove"))
        {
            to_remove = true;
            auto& names{ anim_.get_clip_names() };
            for(const String& other : names)
            {
                if(other == name) continue;
                const AnimClip* other_clip{ anim_.get_clip(other) };
                if(!other_clip || other_clip->next != name) continue;

                AnimClip patched{ *other_clip }; // whoever pointed here lets go
                patched.next.clear();
                anim_.replace_clip(other, patched);
            }
        }

        ImGui::EndPopup();
        return to_remove;
    }
}
