module;

#include <vector>
#include <utility>
#include <unordered_map>
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
        AnimClip* get_clip_mut(const String& name);

        void set_tex_uuid(UUID uuid);
        void emplace_clip(String name, AnimClip clip = {});
        void replace_clip(const String& name, AnimClip clip = {});
        void remove_clip (const String& name);
    private:
        bool load_from(const Path& filepath); // Caller: AssetsManager
        bool save_to(const Path& filepath) const; // Caller: AnimationEditorPanel
    private:
        AssetUUID tex_uuid_;
        AssetResolve resolved_tex_{};

        std::vector<String> clip_names_{}; // for editor
        std::unordered_map<String, AnimClip> clips_{};
    };
}
