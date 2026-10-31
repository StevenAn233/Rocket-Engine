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
    
    static void sync_to_ac(AnimatorComponent& ac,
        const Animation& anim, const AnimPlayState& state)
    {
        if(state.active.empty()) return;

        const AnimClip* clip{ anim.get_clip(state.active) };
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

    void AnimatorSystem::on_update(double dt)
    {
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        AssetsManager& am{ project_->get_assets_manager_mut() };
        align_states();

        for(auto&& [ent, ac] : storage.each())
        {
            AnimPlayState* state{ refresh_state(static_cast<EntityHandle>(ent), ac) };
            if(!state) { ac.curr_tex_handle = asset_handle_null; continue; }
            
            Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) };
            if(anim && !anim->get_tex_uuid().empty())
                ac.curr_tex_handle = anim->get_tex_handle(am).first;
            else ac.curr_tex_handle = asset_handle_null;

            String requested{ ac.has_clip() ? String(ac.get_clip()) : String{} };
            if(requested != state->requested)
            {
                state->set_to(requested);
                if(anim) sync_to_ac(ac, *anim, *state);
            }
            
            if(anim && state->advance(*anim, dt))
                sync_to_ac(ac, *anim, *state);
        }
    }

    void AnimatorSystem::play(EntityHandle handle)
    {
        AnimPlayState* state{ state_of(handle) };
        if(state) state->play();
    }

    void AnimatorSystem::stop(EntityHandle handle)
    {
        AnimPlayState* state{ state_of(handle) };
        if(state) {
            state->stop();
            AssetsManager& am{ project_->get_assets_manager_mut() };
            if(Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) })
            {
                AnimatorComponent& ac{ owner_->registry_->
                    get<AnimatorComponent>(static_cast<entt::entity>(handle)) };
                sync_to_ac(ac, *anim, *state);
            }
        }
    }

    void AnimatorSystem::pause(EntityHandle handle)
    {
        AnimPlayState* state{ state_of(handle) };
        if(state) state->pause();
    }

    void AnimatorSystem::resume(EntityHandle handle)
    {
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

    AnimPlayState* AnimatorSystem::state_of(EntityHandle handle)
    {
        entt::entity ent{ static_cast<entt::entity>(handle) };
        auto& reg{ *(owner_->registry_) };
        if(!reg.all_of<AnimatorComponent>(ent)) return nullptr;
        
        auto& ac{ reg.get<AnimatorComponent>(ent) };
        align_states();
        return refresh_state(handle, ac);
    }

    AnimPlayState* AnimatorSystem::refresh_state
        (EntityHandle handle, AnimatorComponent& ac)
    {
        const Size index{ owner_->registry_->storage<AnimatorComponent>()
            .index(static_cast<entt::entity>(handle)) };
        auto& [owner_handle, state]{ states_[index] };
        if(owner_handle != handle) // slot reused / states_ desynced -> rebuild
        {
            state = {};
            owner_handle = handle;
        }
        if(ac.anim_uuid.empty()) return nullptr;

        AssetsManager& am{ project_->get_assets_manager_mut() };
        auto [_, refreshed]{ am.resolve(state.resolved_anim, ac.anim_uuid) };
        if(refreshed)
        {
            state.stop();
            state.requested.clear();
            state.active.clear();

            const Animation* anim{ am.get_asset<Animation>(state.resolved_anim.handle) };
            if(anim) sync_to_ac(ac, *anim, state);
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
