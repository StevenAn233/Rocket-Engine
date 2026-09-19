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

    void AnimationEditorPanel::selected_clip_editing(AssetsManager& am)
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
        std::memcpy(name_buffer, selected_clip_.raw(),
            std::min(selected_clip_.length(), sizeof(name_buffer) - 1));
        ImGui::SetNextItemWidth(basic_width);
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
        if(ImGui::DragInt2("Cell Size", cell, 1.0f, -8192, 8192, "%d px"))
        {
            clip.cell_size = {
                (cell[0]) == 0 ? 1 : cell[0],
                (cell[1]) == 0 ? 1 : cell[1]
            };
            changed = true;
        }

        ImGui::SameLine();
        layout::vertical_separator(row_height);

        int fps{ static_cast<int>(clip.fps) };
        ImGui::SetNextItemWidth(row_height * 1.33f);
        if(ImGui::DragInt("FPS", &fps, 1.0f, 1, 144))
        {
            clip.fps = static_cast<uint32>(fps);
            changed = true;
        }

        ImGui::SameLine();
        layout::vertical_separator(row_height);

        if(ImGui::Checkbox("Loop", &clip.loop)) changed = true;

        ImGui::SameLine();
        layout::vertical_separator(row_height);

        ImGui::SetNextItemWidth(basic_width);
        if(ImGui::BeginCombo("Next", clip.next.empty() ? "<None>" : clip.next.raw()))
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
        ImGui::Separator();

    // frames editing
        const AssetHandle tex_handle{ anim_.get_tex_handle(am).first };
        if(tex_handle == asset_handle_null)
            { ImGui::TextDisabled("no texture."); return; }

        Texture* texture{ am.get_asset<Texture>(tex_handle) };
        GTexture* gtex{ texture ?
            texture->get_gtexture(GTextureSettings
            {
                .filt = GTexture::FiltFormat::Nearest,
                .wrap = GTexture::WrapFormat::Repeat,
                .srgb = false
            })
            : nullptr };
        CORE_ASSERT(gtex, u8"AnimationEditorPanel: Texture format invalid!");

        const float tex_w{ static_cast<float>(texture->get_width ()) };
        const float tex_h{ static_cast<float>(texture->get_height()) };

        const float cell_w_px{ static_cast<float>
            (std::max(1, std::abs(clip.cell_size.first ))) };
        const float cell_h_px{ static_cast<float>
            (std::max(1, std::abs(clip.cell_size.second))) };

        const float cell_uscale{ static_cast<float>(clip.cell_size.first ) / tex_w };
        const float cell_vscale{ static_cast<float>(clip.cell_size.second) / tex_h };

        const int cols{ static_cast<int>(ceil(1.0f / std::abs(cell_uscale))) };
        const int rows{ static_cast<int>(ceil(1.0f / std::abs(cell_vscale))) };

    // frames
        if(ImGui::SmallButton("~") && !clip.frames.empty())
        {
            clip.frames.clear();
            selected_frame_ = 0;
            changed = true;
        }
        ImGui::SameLine();
        ImGui::Text("Frames: %zu", clip.frames.size());
        
        Size edit_index{ clip.frames.size() };
        int edit_action{ 0 };
        // 1 remove, 2 move left, 3 move right,
        // 4 insert before, 5 insert after

        constexpr float thumb_fit{ 48.0f };
        const float thumb_scale{ thumb_fit / std::max(cell_w_px, cell_h_px) };
        const ImVec2 thumb_size{ cell_w_px * thumb_scale, cell_h_px * thumb_scale };

        const ImGuiStyle& style{ ImGui::GetStyle() };
        const float column_width{ thumb_size.x
            + style.FramePadding.x * 2.0f    // the ImageButton frame
            + style.CellPadding.x  * 2.0f }; // the gap a table column adds
        const float usable_width{ ImGui::GetContentRegionAvail().x
            - style.CellPadding.x * 2.0f };  // the table's own outer padding
        const int frame_columns{ std::max(1, std::min
        (
            static_cast<int>(usable_width / column_width),
            static_cast<int>(clip.frames.size())
        ))};

        if(!clip.frames.empty())
            ImGui::GetWindowDrawList()->AddCallback
                (ImGui_ImplOpenGL3_DisableBindSampler, nullptr);

        if(!clip.frames.empty() && ImGui::BeginTable("##frames",
            frame_columns, ImGuiTableFlags_SizingFixedFit))
        {
            for(Size i{}; i < clip.frames.size(); ++i)
            {
                const std::pair<int, int>& cell{ clip.frames[i] };
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextColumn(); // wraps into the next row on its own

                const ImVec2 uv0
                {
                    static_cast<float>(cell.first) * cell_uscale,
                    static_cast<float>(cell.second + 1) * cell_vscale
                };
                const ImVec2 uv1
                {
                    static_cast<float>(cell.first + 1) * cell_uscale,
                    static_cast<float>(cell.second) * cell_vscale
                };

                if(ImGui::ImageButton("##frame", ImTextureRef
                    (static_cast<ImTextureID>(gtex->get_gal_id())),
                    thumb_size, uv0, uv1)) selected_frame_ = i;

                const float item_width{ ImGui::GetItemRectSize().x };
                if(i == selected_frame_)
                    ImGui::GetWindowDrawList()->AddRect
                    (
                        ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                        IM_COL32(255, 210, 0, 255), 0.0f, 0, 2.0f
                    );

                if(ImGui::BeginPopupContextItem("##frame"))
                {
                    if(ImGui::MenuItem("Insert Before"))
                        { edit_index = i; edit_action = 4; }
                    if(ImGui::MenuItem("Insert After"))
                        { edit_index = i; edit_action = 5; }
                    ImGui::Separator();
                    if(ImGui::MenuItem("Move Left", nullptr, false, i > 0))
                        { edit_index = i; edit_action = 2; }
                    if(ImGui::MenuItem("Move Right", nullptr, false, i + 1 < clip.frames.size()))
                        { edit_index = i; edit_action = 3; }
                    ImGui::Separator();
                    if(ImGui::MenuItem("Remove")) { edit_index = i; edit_action = 1; }
                    ImGui::EndPopup();
                }

            // centre the index
                char index_label[16]{};
                std::snprintf(index_label, sizeof(index_label), "%zu", i);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()
                    + (item_width - ImGui::CalcTextSize(index_label).x) * 0.5f);
                ImGui::TextDisabled("%s", index_label);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if(!clip.frames.empty())
            ImGui::GetWindowDrawList()->AddCallback
                (ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);

    // applied after the loop, the indices in it are only valid before an edit
        if(edit_action != 0 && edit_index < clip.frames.size())
        {
            const auto pos{ clip.frames.begin() + static_cast<std::ptrdiff_t>(edit_index) };
            const std::pair<int, int> frame{ *pos }; // insert may move the storage
            switch(edit_action)
            {
            case 1: clip.frames.erase(pos); break;
            case 2:
                std::swap(*pos, *(pos - 1));
                if(selected_frame_ == edit_index) selected_frame_ = edit_index - 1;
                break;
            case 3:
                std::swap(*pos, *(pos + 1));
                if(selected_frame_ == edit_index) selected_frame_ = edit_index + 1;
                break;
            case 4:
                clip.frames.insert(pos, frame);
                selected_frame_ = edit_index + 1;
                break;
            case 5:
                clip.frames.insert(pos + 1, frame);
                selected_frame_ = edit_index;
                break;
            default: break;
            }
            if(selected_frame_ >= clip.frames.size())
                selected_frame_ = clip.frames.empty() ? 0 : clip.frames.size() - 1;
            changed = true;
        }
        ImGui::Separator();

    // the view controls, zoom is a multiplier over "fit the width"
        if(ImGui::SmallButton("-")) sheet_zoom_ = std::max(0.05f, sheet_zoom_ * 0.8f);
        ImGui::SameLine();
        if(ImGui::SmallButton("+")) sheet_zoom_ = std::min(64.0f, sheet_zoom_ * 1.25f);
        ImGui::SameLine();
        if(ImGui::SmallButton("Fit")) sheet_zoom_ = 1.0f;
        ImGui::SameLine();
        ImGui::TextDisabled("x%.2f", sheet_zoom_);
        if(ImGui::IsItemHovered()) ImGui::SetTooltip
        (
            "Ctrl+wheel zooms, wheel scrolls,\n"
            "shift+wheel scrolls sideways, middle drag pans."
        );

    // sheet: a canvas, not just the texture. Cells past the sheet are legal;
    // the lattice keeps going right and up and the view scrolls and zooms over it.
        ImGui::BeginChild("##sheet_view", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        
        ImGuiIO& io{ ImGui::GetIO() };

        int lat_c_min{}, lat_c_max{ cols };
        int lat_r_min{}, lat_r_max{ rows };
        for(std::pair<int, int> frame : clip.frames)
        {
            lat_c_min = std::min(lat_c_min, frame.first);
            lat_c_max = std::max(lat_c_max, frame.first + 1);
            lat_r_min = std::min(lat_r_min, frame.second);
            lat_r_max = std::max(lat_r_max, frame.second + 1);
        }

    // the zoom comes first, this frame's sizes are a result of it
        const float last_zoom{ sheet_zoom_ };
        if(ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f)
            sheet_zoom_ = std::clamp(sheet_zoom_ * std::pow(1.15f, io.MouseWheel), 0.05f, 64.0f);

        const float fit{ tex_w > 0.0f ? ImGui::GetContentRegionAvail().x / tex_w : 1.0f };
        const float scale{ std::clamp(fit * sheet_zoom_, 0.02f, 64.0f) };
        const float old_scale{ std::clamp(fit * last_zoom, 0.02f, 64.0f) };

        float scroll_dx{}, scroll_dy{};
        if(scale != old_scale)
        {
            const float ratio{ scale / old_scale };
            scroll_dx = (ratio - 1.0f)
                * -static_cast<float>(lat_c_min) * cell_w_px * old_scale;
            scroll_dy = (ratio - 1.0f)
                * static_cast<float>(lat_r_max) * cell_h_px * old_scale;

            ImGui::SetScrollX(ImGui::GetScrollX() + scroll_dx);
            ImGui::SetScrollY(ImGui::GetScrollY() + scroll_dy);
        }

        const float cell_w{ cell_w_px * scale };
        const float cell_h{ cell_h_px * scale };
        const ImVec2 sheet_size{ tex_w * scale, tex_h * scale };
        const ImVec2 canvas_size
        {
            static_cast<float>(lat_c_max - lat_c_min) * cell_w,
            static_cast<float>(lat_r_max - lat_r_min) * cell_h
        };

        const ImVec2 cursor{ ImGui::GetCursorScreenPos() };
        const ImVec2 canvas_origin{ cursor.x - scroll_dx, cursor.y - scroll_dy };
        const ImVec2 sheet_origin
        {
            canvas_origin.x - static_cast<float>(lat_c_min) * cell_w,
            canvas_origin.y + static_cast<float>(lat_r_max) * cell_h - sheet_size.y
        };

        if(ImGui::IsWindowHovered() && !io.KeyCtrl && io.MouseWheel != 0.0f)
        {
            const float step{ ImGui::GetTextLineHeight() * 3.0f };
            if(io.KeyShift) ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseWheel * step);
            else ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseWheel * step);
        }

        ImDrawList* draw{ ImGui::GetWindowDrawList() };

    // behind the see through parts of the texture
        draw->AddRectFilled(canvas_origin,
            ImVec2(canvas_origin.x + canvas_size.x, canvas_origin.y + canvas_size.y),
            IM_COL32(0, 0, 0, 80));

        ImGui::SetCursorScreenPos(canvas_origin);
        draw->AddCallback(ImGui_ImplOpenGL3_DisableBindSampler, nullptr);
        ImGui::Image(ImTextureRef(static_cast<ImTextureID>(gtex->get_gal_id())),
            canvas_size,
            ImVec2(lat_c_min * cell_uscale, lat_r_max * cell_vscale),
            ImVec2(lat_c_max * cell_uscale, lat_r_min * cell_vscale));
        draw->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);

        ImGui::SetCursorScreenPos(canvas_origin);
        ImGui::InvisibleButton("##sheet", canvas_size,
            ImGuiButtonFlags_MouseButtonLeft  |
            ImGuiButtonFlags_MouseButtonMiddle);

        if(ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseDelta.x);
            ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
        }

        for(int col{}; col <= lat_c_max - lat_c_min; ++col)
        {
            const float x{ canvas_origin.x + static_cast<float>(col) * cell_w };
            draw->AddLine(ImVec2(x, canvas_origin.y),
                ImVec2(x, canvas_origin.y + canvas_size.y), IM_COL32(255, 255, 255, 40));
        }
        for(int row{}; row <= lat_r_max - lat_r_min; ++row)
        {
            const float y{ canvas_origin.y + static_cast<float>(row) * cell_h };
            draw->AddLine(ImVec2(canvas_origin.x, y),
                ImVec2(canvas_origin.x + canvas_size.x, y), IM_COL32(255, 255, 255, 40));
        }

        const ImVec2 sheet_max
        {
            sheet_origin.x + sheet_size.x,
            sheet_origin.y + sheet_size.y
        };
        const float mark{ std::min(20.0f,
            std::min(sheet_size.x, sheet_size.y) * 0.25f) };
        constexpr ImU32 extent_color{ IM_COL32(120, 255, 170, 255) };

        draw->AddRect(ImVec2(sheet_origin.x - 1.0f, sheet_origin.y - 1.0f),
            ImVec2(sheet_max.x + 1.0f, sheet_max.y + 1.0f),
            IM_COL32(0, 0, 0, 150), 0.0f, 0, 3.0f);
        draw->AddRect(sheet_origin, sheet_max, extent_color, 0.0f, 0, 2.0f);

        const ImVec2 corners[4]
        {
            sheet_origin,
            ImVec2(sheet_max.x, sheet_origin.y),
            sheet_max,
            ImVec2(sheet_origin.x, sheet_max.y)
        };
        for(int i{}; i < 4; ++i)
        {
            const float dx{ (i == 1 || i == 2) ? -mark : mark };
            const float dy{ (i >= 2) ? -mark : mark };
            draw->AddLine(corners[i], ImVec2(corners[i].x + dx, corners[i].y),
                extent_color, 3.0f);
            draw->AddLine(corners[i], ImVec2(corners[i].x, corners[i].y + dy),
                extent_color, 3.0f);
        }

        const auto cell_rect{ [&](const std::pair<int, int>& cell)
        {
            const ImVec2 pos
            {
                canvas_origin.x + static_cast<float>(cell.first - lat_c_min) * cell_w,
                canvas_origin.y
                    + static_cast<float>(lat_r_max - cell.second - 1) * cell_h
            };
            return std::pair<ImVec2, ImVec2>{ pos, ImVec2(pos.x + cell_w, pos.y + cell_h) };
        }};

        for(Size i{}; i < clip.frames.size(); ++i)
        {
            const auto [lo, hi]{ cell_rect(clip.frames[i]) };
            draw->AddRect(lo, hi, IM_COL32(120, 190, 255, 200), 0.0f, 0, 1.5f);
        }
        if(selected_frame_ < clip.frames.size())
        {
            const auto [lo, hi]{ cell_rect(clip.frames[selected_frame_]) };
            draw->AddRect(lo, hi, IM_COL32(255, 210, 0, 255), 0.0f, 0, 2.5f);
        }

    // the other way round: what the mouse is over -> the cell stored in the file
        const auto cell_at{ [&](ImVec2 mouse, std::pair<int, int>& cell)
        {
            const int col{ lat_c_min + static_cast<int>
                ((mouse.x - canvas_origin.x) / cell_w) };
            const int row{ lat_r_max - 1 - static_cast<int>
                ((mouse.y - canvas_origin.y) / cell_h) };

            if(col < lat_c_min || col >= lat_c_max) return false;
            if(row < lat_r_min || row >= lat_r_max) return false;
            cell = { col, row };
            return true;
        }};

        if(ImGui::IsItemHovered())
        {
            std::pair<int, int> hovered{ 0, 0 };
            if(cell_at(io.MousePos, hovered))
            {
                const auto [lo, hi]{ cell_rect(hovered) };
                draw->AddRectFilled(lo, hi, IM_COL32(255, 255, 255, 80));

                const bool append{ io.KeyShift || clip.frames.empty() };
                if(append) ImGui::SetTooltip (
                    "Cell (%d, %d)\nClick to append.",
                    hovered.first, hovered.second);
                else ImGui::SetTooltip (
                    "Cell (%d, %d)\nClick to set frame %zu.\n"
                    "Click+shift to append.",
                    hovered.first, hovered.second, selected_frame_);

                if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    changed = true;
                    if(append) {
                        clip.frames.push_back(hovered);
                        selected_frame_ = clip.frames.size() - 1;
                    }
                    else clip.frames[selected_frame_] = hovered;
                }
            }
        }
        ImGui::EndChild();

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
