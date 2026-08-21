module;
module ScriptManager;

import Log;
import Scene;
import Project;
import Script;
import Components;
import ScriptRegistry;

namespace rke
{
    ScriptManager::ScriptManager(Scene* owner) : owner_(owner)
        { CORE_ASSERT(owner_, u8"ScriptManager: Owner scene empty!"); }

    void ScriptManager::on_runtime_start()
    {
        auto& registry{ *(owner_->registry_) };
        auto view{ registry.view<NativeScriptComponent>() };
        for(entt::entity ent : view)
            refresh_script(static_cast<uint32>(ent));
    }

    void ScriptManager::on_runtime_stop()
    {
        while(!script_cache_.empty())
        {
            Script* script{ reinterpret_cast<Script*>(script_cache_.back()) };
            if(script) script->on_destroy();
            delete script;
            script_cache_.pop_back();
        }
        auto& registry{ *(owner_->registry_) };
        auto view{ registry.view<NativeScriptComponent>() };
        for(entt::entity ent : view)
            registry.get<NativeScriptComponent>(ent).script_handle = nullptr;
    }

    void ScriptManager::refresh_script(uint32 handle)
    {
        if(!owner_->in_runtime()) return;
        entt::entity ent{ static_cast<entt::entity>(handle) };
        auto& script_com{ owner_->registry_->get<NativeScriptComponent>(ent) };
        const auto& name{ script_com.script_name };
        if(!name.empty()) {
            Script* script{ reinterpret_cast<Script*>
                (owner_->get_owner()->get_script_registry().construct_script(name)) };
            if(script) {
                Entity entity{ owner_->get_entity(handle) };
                if(!entity.valid()) {
                    CORE_ERROR(u8"ScriptManager: Script owner not valid!");
                    script->owner_ = Entity{};
                }
                else script->owner_ = entity;
                script->on_create();
            }
            script_cache_.push_back(static_cast<void*>(script));
            script_com.script_handle = script;
        }
    }
}
