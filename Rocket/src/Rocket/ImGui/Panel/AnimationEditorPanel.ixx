module;
export module AnimationEditorPanel;

import Panel;

export namespace rke
{

    class AnimationEditorPanel : public Panel
    {
    /*
    public:
        AnimationClipEditorPanel(String name);
        ~AnimationClipEditorPanel();

        // Opens the given .rkanim asset (by its AssetUUID) for editing.
        void open(UUID clip_set_uuid);
        inline void close_editor() { hide(); }
        // writes the working copy back to disk and reloads the managed asset
        void save();

        inline bool is_editing() const { return editing_; }
        inline UUID get_editing_uuid() const { return editing_uuid_; }
    private:
        void on_imgui_render() override;

        void draw_header();
        void draw_settings_and_clip_list();
        void draw_clip_params();
        void draw_sheet_and_frames();
        void draw_transport();

        Project* project();
        AssetsManager& assets_manager();
        // snapshots the managed asset into working_copy_
        bool reload_asset_context();

        Texture* resolve_sheet_texture();
        GTexture* resolve_sheet_gtex();
    private:
        bool editing_{ false };
        UUID editing_uuid_{};

        Scope<Animation> working_copy_{};

        // selection state (valid only while editing_)
        int32 selected_clip_{ -1 };
        int32 selected_frame_{ -1 };

        // hovered cell in the sheet picker
        std::pair<int, int> hovered_cell_{ -1, -1 };
        // hovered image-picker space, pixel uv range of the sheet (bottom-left origin)
        glm::vec2 sheet_uv_offset_{ 0.0f };
        glm::vec2 sheet_uv_scale_{ 1.0f };

        // rename buffer for the selected clip
        std::array<char, 128> clip_name_buffer_{};
        bool clip_name_editing_{ false };
        int32 clip_name_edited_clip_{ -1 };

        // preview transport
        bool preview_playing_{ false };
        float preview_time_{ 0.0f };
        int32 preview_clip_{ -1 }; // which clip the transport plays (-1 = follows selection)
    */
    };
}
