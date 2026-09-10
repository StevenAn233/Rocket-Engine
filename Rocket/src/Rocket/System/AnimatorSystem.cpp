module;
module AnimatorSystem;

import Log;
import Scene;
import String;
import Project;
import Components;
import AssetsManager;
import Animation;

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

        Size index{};
        for(auto&& [ent, ac] : storage.reach())
        {
        #ifdef RKE_DEBUG
            CORE_ASSERT(index == storage.index(ent),
                u8"AnimatorSystem: Indices are not matching: {}, {}!",
                index, storage.index(ent));
        #endif
            AnimPlayState* state{ refresh_state
                (static_cast<EntityHandle>(ent), ac, index++) };
            if(!state) { ac.curr_tex_handle = asset_handle_null; continue; }
            
            Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) };
            if(anim && !anim->get_tex_uuid().empty())
                ac.curr_tex_handle = anim->get_tex_handle(am).first;
            else ac.curr_tex_handle = asset_handle_null;

            String requested{ ac.has_clip() ? String(ac.get_clip()) : String{} };
            if(requested != state->requested)
            {
                state->requested = requested;
                rewind_to_start(*state);
                if(anim) update_animator_component(ac, *anim, *state);
            }
            
            if(state->playing && !(state->paused) && anim)
                if(advance(*anim, *state, dt)) // <- active not empty
                    update_animator_component(ac, *anim, *state);
        }
    }

    void AnimatorSystem::play(EntityHandle handle)
    {
        AnimPlayState* state{ get_state_from(handle) };
        if(!state) return;
        if(!state->playing && !state->active.empty() && at_start(*state))
        {
            state->playing = true;
            state->paused = false;
        }
    }

    void AnimatorSystem::stop(EntityHandle handle)
    {
        AnimPlayState* state{ get_state_from(handle) };
        if(!state) return;
        state->playing = state->paused = false;

        rewind_to_start(*state);
        AssetsManager& am{ project_->get_assets_manager_mut() };
        if(Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) })
        {
            AnimatorComponent& ac{ owner_->registry_->
                get<AnimatorComponent>(static_cast<entt::entity>(handle)) };
            update_animator_component(ac, *anim, *state);
        }
    }

    void AnimatorSystem::pause(EntityHandle handle)
    {
        AnimPlayState* state{ get_state_from(handle) };
        if(state && state->playing) state->paused = true;
    }

    void AnimatorSystem::resume(EntityHandle handle)
    {
        AnimPlayState* state{ get_state_from(handle) };
        if(state) state->paused = false;
    }

    bool AnimatorSystem::playing(EntityHandle handle)
    {
        AnimPlayState* state{ get_state_from(handle) };
        if(state) return state->playing;
        return false;
    }

    bool AnimatorSystem::paused(EntityHandle handle)
    {
        AnimPlayState* state{ get_state_from(handle) };
        if(state) return state->paused;
        return false;
    }

    AnimatorSystem::AnimPlayState* AnimatorSystem::get_state_from(EntityHandle handle)
    {
        entt::entity ent{ static_cast<entt::entity>(handle) };
        auto& reg{ *(owner_->registry_) };
        if(!reg.all_of<AnimatorComponent>(ent)) return nullptr;
        
        Size index{ reg.storage<AnimatorComponent>().index(ent) };
        auto& ac{ reg.get<AnimatorComponent>(ent) };
        align_states();
        return refresh_state(handle, ac, index);
    }

    AnimatorSystem::AnimPlayState* AnimatorSystem::refresh_state
        (EntityHandle handle, AnimatorComponent& ac, Size index)
    {
        AnimPlayState& state{ states_[index] };
        if(state.owner != handle) // slot reused / states_ desynced -> rebuild
        {
            state = AnimPlayState{};
            state.owner = handle;
        }
        if(ac.anim_uuid.empty()) return nullptr;

        AssetsManager& am{ project_->get_assets_manager_mut() };
        auto [_, refreshed]{ am.resolve(state.resolved_anim, ac.anim_uuid) };
        if(refreshed)
        {
            rewind_to_start(state);
            state.requested.clear();
            state.active.clear();
            state.playing = state.paused = false;

            if(Animation* anim{ am.get_asset<Animation>(state.resolved_anim.handle) })
            {
                AnimatorComponent& ac{ owner_->registry_->
                    get<AnimatorComponent>(static_cast<entt::entity>(handle)) };
                update_animator_component(ac, *anim, state);
            }
        }
        return &state;
    }

    void AnimatorSystem::align_states()
    {
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        if(states_.size() != storage.size())
            states_.resize(storage.size(), {});
    }

    void AnimatorSystem::rewind_to_start(AnimPlayState& state)
    {
        state.active = state.requested;
        state.frame_index = 0;
        state.acc = 0.0;
    }

    bool AnimatorSystem::at_start(AnimPlayState& state)
    {
        return state.requested == state.active &&
               state.frame_index == 0 &&
               state.acc == 0.0;
    }

    bool AnimatorSystem::advance(Animation& anim, AnimPlayState& state, double dt)
    {
        constexpr double max_dt{ 0.5 };
        constexpr uint32  fuse { 128 };
        
        state.acc += dt;
        if(state.acc > max_dt) state.acc = max_dt;

        bool advanced{ false };
        for(uint32 guard{}; guard < fuse; ++guard)
        {
            if(state.active.empty()) break;
            const AnimClip* clip{ anim.get_clip(state.active) };
            if(!clip || clip->frames.empty()) break;

            const double spf{ 1.0 / (clip->fps ? clip->fps : 1u) };
            if(state.acc < spf) return advanced;

            state.acc -= spf;
            advanced = true;

            if(state.frame_index < clip->frames.size() - 1)
            {
                ++state.frame_index;
                continue;
            }

            state.frame_index = 0;
            if(clip->loop) continue;
            state.active = clip->next;
        }
        state.acc = 0.0;
        state.playing = false;
        return advanced;
    }

    void AnimatorSystem::update_animator_component
        (AnimatorComponent& ac, Animation& anim, AnimPlayState& state)
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

    void AnimatorSystem::on_anim_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.animator_system_, u8"AnimatorSystem: Null!");

        auto& states { ctx.animator_system_->states_ };
        auto& storage{ reg.storage<AnimatorComponent>() };
    // this callback fires *before* EnTT pops the element: mirror the upcoming
    // swap-and-pop, but only while both arrays are actually in sync.
    // otherwise leave it to the per-frame owner check, which heals itself.
        if(states.size() != storage.size()) return;
        if(!storage.contains(ent)) return;

        const Size index{ storage.index(ent) };
        if(index + 1u != states.size())
            *(states.begin() + index) = std::move(states.back());
        states.pop_back();
    }
}
