module;

#include <utility>
#include "rke_macros.h"

export module AnimationEditorPanel;

import Panel;
import Animation;
import String;
import Types;
import AssetAccess;
import AssetsManager;

export namespace rke
{
    class RKE_API AnimationEditorPanel : public Panel
    {
    public:
        AnimationEditorPanel(String name);
        ~AnimationEditorPanel() override;
    private:
        void on_imgui_render() override;

        void clear();
        void open(AssetUUID uuid);

        void clips_managing(AssetsManager& am);
        void selected_clip_editing(AssetsManager& am); // TO MODIFY

        void anim_popup(AssetsManager& am);
        bool clip_popup(const String& name);
    private:
        Animation anim_{}; // working copy, written back on demand
        AssetUUID asset_uuid_{ 0 };
        Path asset_path_{};
        bool modified_{ false };

    // states
        String selected_clip_{};
        Size   selected_frame_{ 0 };
        bool   preview_playing_{ false };
        float  preview_time_{ 0.0f };
    };
}
