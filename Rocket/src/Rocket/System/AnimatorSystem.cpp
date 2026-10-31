module;
module AnimatorSystem;

import Log;
import Scene;
import String;
import Project;
import Components;
import AssetsManager;
import Animation;

namespace {
    using namespace rke;
    
    static void update_sprite(AnimatorComponent& ac,
        const Animation* anim, const AnimPlayState& state)
    {
        const AnimClip* clip{ (anim && !state.active.empty())
            ? anim->get_clip(state.active) : nullptr };
        if(!clip || clip->frames.empty())
        {
            ac.curr_cell_coords = { 0, 0 };
            return;
        }
        ac.curr_cell_size = clip->cell_size;
        CORE_ASSERT(state.frame_index < clip->frames.size(),
            u8"AnimatorSystem: Frame index out of bound!");
        ac.curr_cell_coords = clip->frames[state.frame_index];
    }
}

namespace rke
{
    AnimatorSystem::AnimatorSystem(Scene* owner) : owner_(owner)
    {
        CORE_ASSERT(owner_, u8"AnimatorSystem: Owner scene null!");
        project_ = owner_->get_owner();
        CORE_ASSERT(project_, u8"AnimatorSystem: Project null!");
        owner_->registry_->on_destroy<AnimatorComponent>()
            .connect<&on_anim_com_destroy>();
    }

    void AnimatorSystem::on_runtime_start()
    {
        align_states();
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        for(auto&& [ent, ac] : storage.each())
            refresh_state(static_cast<EntityHandle>(ent), ac);
    }
    void AnimatorSystem::on_runtime_stop() {}

    void AnimatorSystem::on_update(double dt)
    {
        align_states();
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        AssetsManager& am{ project_->get_assets_manager_mut() };
        for(auto&& [ent, ac] : storage.each())
        {
            AnimPlayState* state{ refresh_state(static_cast<EntityHandle>(ent), ac) };
            if(!state || !owner_->in_runtime()) continue;

            Animation* anim{ am.get_asset<Animation>(ac.resolved_anim.handle) };
            if(anim && state->advance(*anim, dt)) update_sprite(ac, anim, *state);
        }
    }

    void AnimatorSystem::play(EntityHandle handle)
    {
        if(!owner_->in_runtime()) return;
        AnimPlayState* state{ state_of(handle) };
        if(state) state->play();
    }

    void AnimatorSystem::stop(EntityHandle handle)
    {
        if(!owner_->in_runtime()) return;
        AnimPlayState* state{ state_of(handle) };
        if(state) {
            state->stop();
            AssetsManager& am{ project_->get_assets_manager_mut() };
            AnimatorComponent& ac{ owner_->registry_->
                get<AnimatorComponent>(static_cast<entt::entity>(handle)) };
            Animation* anim{ am.get_asset<Animation>(ac.resolved_anim.handle) };
            update_sprite(ac, anim, *state);
        }
    }

    void AnimatorSystem::pause(EntityHandle handle)
    {
        if(!owner_->in_runtime()) return;
        AnimPlayState* state{ state_of(handle) };
        if(state) state->pause();
    }

    void AnimatorSystem::resume(EntityHandle handle)
    {
        if(!owner_->in_runtime()) return;
        AnimPlayState* state{ state_of(handle) };
        if(state) state->resume();
    }

    bool AnimatorSystem::playing(EntityHandle handle)
    {
        AnimPlayState* state{ state_of(handle) };
        if(state) return state->playing;
        return false;
    }

    bool AnimatorSystem::paused(EntityHandle handle)
    {
        AnimPlayState* state{ state_of(handle) };
        if(state) return state->paused;
        return false;
    }

    AnimPlayState* AnimatorSystem::state_of(EntityHandle handle) // silent
    {
        const Size index{ owner_->registry_->storage<AnimatorComponent>()
            .index(static_cast<entt::entity>(handle)) };
        if(index >= states_.size()) return nullptr;
        
        auto& [owner_handle, state]{ states_[index] };
        if(owner_handle != handle) return nullptr;
        
        return &state;
    }

    AnimPlayState* AnimatorSystem::refresh_state
        (EntityHandle handle, AnimatorComponent& ac)
    {
        const Size index{ owner_->registry_->storage<AnimatorComponent>()
            .index(static_cast<entt::entity>(handle)) };
        if(index >= states_.size())
        {
            CORE_ERROR(u8"AnimatorSystem: Index out of bound!");
            return nullptr;
        }
        auto& [owner_handle, state]{ states_[index] };
        if(owner_handle != handle) // slot reused / states_ desynced -> rebuild
        {
            state = {};
            owner_handle = handle;
        }

        bool to_update_sprite{ false };
        AssetsManager& am{ project_->get_assets_manager_mut() };
        auto [_, refreshed]{ am.resolve(ac.resolved_anim, ac.anim_uuid) };
        if(refreshed)
        {
            state.stop();
            state.requested.clear();
            state.active.clear();

            to_update_sprite = true;
        }
        String requested{ ac.has_clip() ? String(ac.get_clip()) : String{} };
        if(requested != state.requested)
        {
            state.set_to(requested);
            to_update_sprite = true;
        }

        if(to_update_sprite)
        {
            Animation* anim{ am.get_asset<Animation>(ac.resolved_anim.handle) };
            update_sprite(ac, anim, state);
        }
        return &state;
    }

    void AnimatorSystem::align_states()
    {
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        if(states_.size() != storage.size())
            states_.resize(storage.size(), {});
    }

    void AnimatorSystem::on_anim_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.animator_system_, u8"AnimatorSystem: Null!");

        auto& states { ctx.animator_system_->states_ };
        auto& storage{ reg.storage<AnimatorComponent>() };

        if(states.size() != storage.size()) return;
        if(!storage.contains(ent)) return;

        const Size index{ storage.index(ent) };
        if(index + 1u != states.size())
            states[index] = std::move(states.back());
        states.pop_back();
    }
}
