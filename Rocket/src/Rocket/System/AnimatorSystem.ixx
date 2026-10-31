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
    private:
        void align_states(); // keep states size == storage size
        AnimPlayState* state_of(EntityHandle handle);
        AnimPlayState* refresh_state(EntityHandle handle, AnimatorComponent& ac);

        static void on_anim_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        Project* project_{};
        std::vector<std::pair<EntityHandle, AnimPlayState>> states_{};
    };
}
