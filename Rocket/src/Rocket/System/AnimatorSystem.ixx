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
    struct AnimPlayState
    {
        EntityHandle owner{ entity_handle_null }; // slot identity (self-healing)
        AssetResolve resolved_anim{};
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
            rewind_to_start(); // may modify
        }

        inline void pause() { if(playing) paused = true; }
        inline void resume() { paused = false; }

        bool advance(const Animation& anim, double dt);
    };

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
        std::vector<AnimPlayState> states_{};
    };
}
