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

    void AnimatorSystem::on_runtime_start()
    {
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        states_.resize(storage.size());
    }

    void AnimatorSystem::on_runtime_stop() { states_.clear(); }

    void AnimatorSystem::on_update(double dt)
    {
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        AssetsManager& am{ project_->get_assets_manager_mut() };
        for(Size index{}; index < storage.size(); index++)
        {
            AnimatorComponent& ac{ *(storage.begin() + index) };
            RuntimeState* state{ get_or_emplace_state(index) };
            if(!state) { ac.curr_tex_handle = asset_handle_null; continue; }
            
            Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) };
            if(anim && !anim->get_tex_uuid().empty())
                ac.curr_tex_handle = anim->get_tex_handle(am).first;
            else ac.curr_tex_handle = asset_handle_null;

            String requested{ ac.has_clip() ? String(ac.get_clip_name()) : String{} };
            if(requested != state->requested)
            {
                state->requested = requested;
                rewind_to_start(*state);
                if(state->active.empty()) state->playing = false;
                else update_animator_component(ac, *anim, *state);
            }
            
            if(state->playing && !(state->paused) && anim)
                if(advance(*anim, *state, dt)) // <- active not empty
                    update_animator_component(ac, *anim, *state);
        }
    }

    void AnimatorSystem::play(EntityHandle handle)
    {
        RuntimeState* state{ check_and_get_state(handle) };
        if(!state) return;

        rewind_to_start(*state);
        if(state->active.empty()) { state->playing = false; return; }

        AssetsManager& am{ project_->get_assets_manager_mut() };
        Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) };
        CORE_ASSERT(anim, u8"AnimatorSystem: Animation null!");
        AnimatorComponent& ac{ owner_->registry_
            ->get<AnimatorComponent>(static_cast<entt::entity>(handle)) };
        update_animator_component(ac, *anim, *state);

        if(state->active_clip_invalid)
            { state->playing = false; state->paused = false; }
        else { state->playing = true; state->paused = false; }
    }

    void AnimatorSystem::stop(EntityHandle handle)
    {
        RuntimeState* state{ check_and_get_state(handle) };
        if(state) state->playing = state->paused = false;
    }

    void AnimatorSystem::pause(EntityHandle handle)
    {
        RuntimeState* state{ check_and_get_state(handle) };
        if(state && state->playing) state->paused = true;
    }

    void AnimatorSystem::resume(EntityHandle handle)
    {
        RuntimeState* state{ check_and_get_state(handle) };
        if(state) state->paused = false;
    }

    Animation* AnimatorSystem::active_anim(EntityHandle handle)
    {
        RuntimeState* state{ check_and_get_state(handle) };
        if(!state) return nullptr;
        AssetsManager& am{ project_->get_assets_manager_mut() };
        return am.get_asset<Animation>(state->resolved_anim.handle);
    }

    std::pair<String, bool> AnimatorSystem::active_clip(EntityHandle handle)
    {
        using namespace literals;
        RuntimeState* state{ check_and_get_state(handle) };
        if(state && !state->active.empty())
        {
            if(state->active_clip_invalid) return { u8"<Invalid Clip>"_s, false };
            return { state->active, true };
        }
        return { u8"<No Clip>"_s, false };
    }

    AnimatorSystem::RuntimeState* AnimatorSystem::check_and_get_state(EntityHandle handle)
    {
        entt::entity ent{ static_cast<entt::entity>(handle) };
        auto& reg{ *(owner_->registry_) };
        if(!reg.all_of<AnimatorComponent>(ent)) return nullptr;
        Size index{ reg.storage<AnimatorComponent>().index(ent) };
        return get_or_emplace_state(index);
    }

    AnimatorSystem::RuntimeState* AnimatorSystem::get_or_emplace_state(Size index)
    {
        auto& storage{ owner_->registry_->storage<AnimatorComponent>() };
        while(states_.size() < storage.size()) states_.emplace_back();

        AnimatorComponent& ac{ *(storage.begin() + index) };
        if(ac.anim_uuid.empty()) return nullptr;

        RuntimeState& state{ *(states_.begin() + index) };
        AssetsManager& am{ project_->get_assets_manager_mut() };
        auto [_, refreshed]{ am.resolve(state.resolved_anim, ac.anim_uuid) };
        if(refreshed) {
            state.requested.clear();
            state.active.clear();
            state.playing = state.paused = false;
            state.frame_index = 0;
            state.acc = 0.0;
            bool active_clip_invalid = false;
        }
        return &state;
    }

    void rke::AnimatorSystem::rewind_to_start(RuntimeState& state)
    {
        state.active = state.requested;
        state.frame_index = 0;
        state.acc = 0.0;
    }

    bool AnimatorSystem::advance(Animation& anim, RuntimeState& state, double dt)
    {
        constexpr double max_dt{ 0.5 };
        constexpr uint32 fuse{ 128 };
        if(state.active.empty()) { state.playing = false; return false; }

        state.acc += dt;
        if(state.acc > max_dt) state.acc = max_dt;

        bool advanced{ false };
        for(uint32 guard{}; guard < fuse; ++guard)
        {
            const AnimClip* clip{ anim.get_clip(state.active) };
            if(!clip || clip->frames.empty())
            {
                CORE_ERROR(u8"AnimatorSystem: Clip invalid to play!");
                state.acc = 0.0;
                state.playing = false;
                state.active_clip_invalid = true;
                return advanced;
            }

            const double spf{ 1.0 / clip->fps };
            if(state.acc < spf) return advanced;

            state.acc -= spf;
            advanced = true;

            if(state.frame_index < clip->frames.size() - 1)
                { ++state.frame_index; continue; }

            if(clip->loop) { state.frame_index = 0; continue; }
            if(!clip->next.empty())
            {
                state.active = clip->next;
                state.frame_index = 0; continue;
            }
            state.playing = false; state.acc = 0.0;
            return true;
        }
        return advanced;
    }

    void AnimatorSystem::update_animator_component
        (AnimatorComponent& ac, Animation& anim, RuntimeState& state)
    {
        if(state.active.empty() || state.active_clip_invalid) return;
        const AnimClip* clip{ anim.get_clip(state.active) };
        if(clip && !clip->frames.empty())
        {
            ac.curr_cell_size = clip->cell_size;
            ac.curr_cell_coords = clip->frames[state.frame_index];
        }
        else state.active_clip_invalid;
    }

    void AnimatorSystem::on_anim_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.animator_system_, u8"AnimatorSystem: Null!");

        auto& sys{ *ctx.animator_system_ };
        if(sys.states_.empty()) return;

        auto& storage{ reg.storage<AnimatorComponent>() };
        Size index{ storage.index(ent) };
        CORE_ASSERT(index < sys.states_.size(), u8"AnimatorComponent: State out of bound!");

    // swap and pop
        *(sys.states_.begin() + index) = std::move(sys.states_.back());
        sys.states_.pop_back();
    }
}
