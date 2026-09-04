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
        auto view{ owner_->registry_->view<AnimatorComponent>() };
        for(entt::entity ent : view)
            find_state(static_cast<uint32>(ent));
    }

    void AnimatorSystem::on_runtime_stop() { states_.clear(); }

    void AnimatorSystem::on_update(double dt)
    {
        AssetsManager& am{ project_->get_assets_manager_mut() };
        auto view{ owner_->registry_->view<AnimatorComponent>() };
        for(entt::entity ent : view)
        {
            uint32 handle{ static_cast<uint32>(ent) };

            RuntimeState* state{ find_state(handle) };
            if(!state) continue;

            auto& ac{ view.get<AnimatorComponent>(ent) };
            Animation* anim{ am.get_asset<Animation>(state->resolved_anim.handle) };
            if(anim && !anim->get_tex_uuid().empty())
            {
                auto [handle, refreshed]{ anim->get_tex_handle(am) };
                ac.curr_tex_handle = handle;
                if(refreshed)
                {
                    Texture* tex{ am.get_asset<Texture>(handle) };
                    ac.curr_cell_size = tex ?
                        std::pair<int, int>
                        (
                            int(tex->get_width ()),
                            int(tex->get_height())
                        ) :
                        std::pair<int, int>(1, 1);
                    ac.curr_cell_coords = { 0, 0 };
                }
            }
            else ac.curr_tex_handle = asset_handle_null;

            String req{ ac.has_clip() ? String(ac.get_clip_name()) : String{} };
            if(req != state->requested)
            {
                state->requested   = req;
                state->active      = req;
                state->frame_index = 0;
                state->acc         = 0.0;
                if(req.empty()) state->playing = false;
            }
            
            bool advanced{ false };
            if(state->playing && !(state->paused) && anim)
                advanced = advance(*anim, *state, dt);

            if(advanced) // state->active not empty
            {
                const AnimClip* clip{ anim->get_clip(state->active) };
                if(!clip || clip->frames.empty()) {
                    Texture* tex{ am.get_asset<Texture>(ac.curr_tex_handle) };
                    ac.curr_cell_size = tex ?
                        std::pair<int, int>
                        (
                            int(tex->get_width ()),
                            int(tex->get_height())
                        ) :
                        std::pair<int, int>(1, 1);
                    ac.curr_cell_coords = { 0, 0 };
                } else {
                    ac.curr_cell_size = clip->cell_size;
                    ac.curr_cell_coords = clip->frames[state->frame_index];
                }
            }
        }
    }

    void AnimatorSystem::play(uint32 handle)
    {
        RuntimeState* state{ find_state(handle) };
        if(!state) return;
        state->playing = true;
        state->paused = false;

        state->active = state->requested;
        state->frame_index = 0;
        state->acc = 0.0;
    }

    void AnimatorSystem::stop(uint32 handle)
    {
        RuntimeState* state{ find_state(handle) };
        if(!state) return;
        state->playing = false;
        state->paused  = false;
    }

    void AnimatorSystem::pause(uint32 handle)
    {
        RuntimeState* state{ find_state(handle) };
        if(!state) return;
        if(state->playing) state->paused = true;
    }

    void AnimatorSystem::resume(uint32 handle)
    {
        RuntimeState* state{ find_state(handle) };
        if(state) state->paused = false;
    }

    AnimatorSystem::RuntimeState* AnimatorSystem::find_state(uint32 handle)
    {
        // drop stale entries whose entity is gone / no longer animatable
        Entity entity{ owner_->get_entity(handle) };
        if(!entity.valid() || !entity.has<AnimatorComponent>())
            { states_.erase(handle); return nullptr; }

        auto& ac{ entity.get_mut<AnimatorComponent>() };
        if(ac.anim_uuid.empty()) { states_.erase(handle); return nullptr; }

        auto it{ states_.find(handle) };
        if(it == states_.end()) it = states_.emplace(handle, RuntimeState{}).first;
        auto& state{ it->second };

        AssetsManager& am{ project_->get_assets_manager_mut() };
        auto [_, refreshed]{ am.resolve(state.resolved_anim, ac.anim_uuid) };
        if(refreshed) {
            state.requested.clear();
            state.active.clear();
            state.frame_index = 0; state.acc = 0.0;
            state.playing = state.paused = false;
        }
        return &state;
    }

    bool AnimatorSystem::advance(Animation& anim, RuntimeState& state, double dt)
    {
        constexpr double max_dt{ 0.5 };
        constexpr uint32 fuse{ 128 };
        if(state.active.empty()) return false;

        state.acc += dt;
        if(state.acc > max_dt) state.acc = max_dt;

        bool advanced{ false };
        for(uint32 guard{}; guard < fuse; ++guard)
        {
            const AnimClip* clip{ anim.get_clip(state.active) };
            if(!clip || clip->frames.empty())
            {
                CORE_WARN(u8"AnimatorSystem: Clip null or invalid to play!");
                state.acc = 0.0;
                state.playing = false;
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

    void AnimatorSystem::on_anim_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.animator_system_, u8"AnimatorSystem: Null!");

        auto& sys{ *ctx.animator_system_ };
        if(sys.states_.empty()) return;

        auto it{ sys.states_.find(static_cast<uint32>(ent)) };
        CORE_ASSERT(it != sys.states_.end(), u8"AnimatorSystem: Entity not found!");
        sys.states_.erase(it);
    }
}
