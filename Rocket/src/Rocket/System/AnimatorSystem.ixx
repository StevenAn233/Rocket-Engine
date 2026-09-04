module;

#include <unordered_map>
#include <entt/entt.hpp>
namespace rke { class Scene; class Project; }

export module AnimatorSystem;

import Types;
import String;
import AssetsManager;
import AssetAccess;
import Animation;

export namespace rke
{
    class AnimatorSystem
    {
    public:
        AnimatorSystem(Scene* owner);
        ~AnimatorSystem() = default;

        AnimatorSystem(const AnimatorSystem&) = delete;
        AnimatorSystem(AnimatorSystem&&) = delete;
        AnimatorSystem& operator=(const AnimatorSystem&) = delete;
        AnimatorSystem& operator=(AnimatorSystem&&) = delete;

        void on_runtime_start();
        void on_runtime_stop ();
        void on_update(double dt);

        void play(uint32 entity);
        void stop(uint32 entity);
        void pause (uint32 entity);
        void resume(uint32 entity);
    private:
        struct RuntimeState
        {
            AssetResolve resolved_anim{};
            bool playing{ false }, paused{ false };

            String requested{}; // clip name
            String active   {}; // clip name
            Size frame_index{};
            double acc{};
        };

        RuntimeState* find_state(uint32 handle); // will refresh state automatically
        bool advance(Animation& anim, RuntimeState& state, double dt);

        static void on_anim_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        Project* project_{};
        std::unordered_map<uint32, RuntimeState> states_{};
    };
}
