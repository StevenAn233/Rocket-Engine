module;

#include <unordered_map>
#include <entt/entt.hpp>
namespace rke
{
    class Project;
    class Scene;
}

export module AnimatorSystem;

import Types;
import String;
import EntityAccess;
import AssetsManager;
import AssetAccess;
import Animation;
import Components;

export namespace rke
{
    class AnimatorSystem
    {
    public:
        friend class Scene;

        struct RuntimeState
        {
            AssetResolve resolved_anim{};
            bool playing{ false }, paused{ false };

            String requested{}; // clip name
            String active   {}; // clip name
            Size frame_index{};
            double acc{};
            bool active_clip_invalid{ false };
        };

        AnimatorSystem(Scene* owner);
        ~AnimatorSystem() = default;

        AnimatorSystem(const AnimatorSystem&) = delete;
        AnimatorSystem(AnimatorSystem&&) = delete;
        AnimatorSystem& operator=(const AnimatorSystem&) = delete;
        AnimatorSystem& operator=(AnimatorSystem&&) = delete;

        void on_update(double dt);

        void play(EntityHandle handle);
        void stop(EntityHandle handle);
        void pause (EntityHandle handle);
        void resume(EntityHandle handle);

        bool playing(EntityHandle handle);
        bool paused (EntityHandle handle);

        std::pair<String, bool> active_clip(EntityHandle handle);
    private:
        RuntimeState* check_and_get_state(EntityHandle handle);
        RuntimeState* get_or_emplace_state(Size index);

        void rewind_to_start(RuntimeState& state);
        bool advance(Animation& anim, RuntimeState& state, double dt);

        void update_animator_component(AnimatorComponent& ac,
            Animation& anim, RuntimeState& state);

        static void on_anim_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        Project* project_{};
        std::vector<RuntimeState> states_{};
    };
}
