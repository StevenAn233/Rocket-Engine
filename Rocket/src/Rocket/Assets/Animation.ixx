module;

#include <vector>
#include <utility>
#include <map>
#include <unordered_set>
#include "rke_macros.h"

export module Animation;

import Time;
import Types;
import String;
import Path;
import UUID;
import HeapManager;
import ConfigProxy;
import AssetAccess;

export namespace rke
{
    // One named, playable clip inside an .rkanim asset set.
    struct RKE_API AnimClip
    {
        std::pair<int, int> cell_size{ 1, 1 };
        std::vector<std::pair<int, int>> frames{};
        uint32 fps{ 12 };
        bool loop{ true };
        String next{}; // auto-transition after finishing (empty = none)
    };

    class RKE_API Animation
    {
    public:
        friend class AssetsManager;
        friend class AnimatorSystem;
        friend class AnimationEditorPanel;

        Animation();
        Animation(const Animation&) = default;
        Animation& operator=(const Animation&) = default;
        Animation(Animation&&) noexcept = default;
        Animation& operator=(Animation&&) noexcept = default;
        
        inline const std::vector<String>& get_clip_names() const { return clip_names_; }
        inline AssetUUID get_tex_uuid() const { return tex_uuid_; }

        std::pair<AssetHandle, bool> get_tex_handle(AssetsManager& am);
        const AnimClip* get_clip(const String& name) const;

        bool failed_contains(const String& name) const;
    private:
        bool load_from(const Path& filepath); // Caller: AssetsManager
        bool save_to(const Path& filepath) const; // Caller: AnimationEditorPanel

        void emplace_clip(String name, AnimClip clip = {});
        void replace_clip(const String& name, AnimClip clip = {});
        void remove_clip (const String& name);
    private:
        AssetUUID tex_uuid_;
        AssetResolve resolved_tex_{};

        std::vector<String> clip_names_{}; // for editor
        std::map<String, AnimClip> clips_{};
        mutable std::unordered_set<String> failed_{};
    };

    struct AnimPlayState
    {
        bool playing{ false };
        bool paused { false };

        String requested{}; // clip name
        String active   {}; // clip name
        Size frame_index{};
        double acc{};

        inline void rewind_to_start()
        {
            active = requested;
            frame_index = 0; acc = 0.0;
        }

        inline bool at_start() const
        {
            return requested == active &&
                frame_index == 0 && acc == 0.0;
        }

        inline void set_to(String clip_name)
        {
            requested = std::move(clip_name);
            rewind_to_start();
        }

        inline void play()
        {
            if(!playing && !active.empty() && at_start())
            {
                playing = true;
                paused = false;
            }
        }

        inline void stop()
        {
            playing = paused = false;
            rewind_to_start();
        }

        inline void pause() { if(playing) paused = true; }
        inline void resume() { paused = false; }

        bool advance(const Animation& anim, double dt);
    };
}
