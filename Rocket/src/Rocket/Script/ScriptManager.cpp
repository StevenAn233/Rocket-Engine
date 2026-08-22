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
        for(auto& [handle, script] : script_cache_)
            destroy_script(std::move(script), handle);
        script_cache_.clear();
    }

    void ScriptManager::refresh_script(uint32 handle)
    {
        if(!owner_->in_runtime()) return;
        if(script_cache_.contains(handle))
            destroy_script(std::move(script_cache_[handle]), handle);
        script_cache_[handle] = create_script(handle);
    }

    Scope<Script> ScriptManager::create_script(uint32 handle)
    {
    // check owner entity
        Entity entity{ owner_->get_entity(handle) };
        if(!entity.valid()) {
            CORE_ERROR(u8"ScriptManager: Script owner not valid!");
            return nullptr;
        }
    // check script name
        auto& script_com{ entity.get_mut<NativeScriptComponent>() };
        const auto& name{ script_com.script_name };
        if(name.empty()) return nullptr;
        
    // create script
        Scope<Script> script{ owner_->get_owner()
            ->get_script_registry().construct_script(name) };
        if(!script) {
            CORE_ERROR(u8"ScriptManager: Failed to create script!");
            return nullptr;
        }
        script->on_create();
    // set mutual refs
        script_com.script_handle = script.get();
        script->owner_ = entity;
        
        return script;
    }

    void ScriptManager::destroy_script(Scope<Script> script, uint32 handle)
    {
    // check owner entity
        Entity entity{ owner_->get_entity(handle) };
        if(!entity.valid()) {
            CORE_ERROR(u8"ScriptManager: Script owner not valid!");
            return;
        }
        auto& script_com{ entity.get_mut<NativeScriptComponent>() };
        if(script_com.script_handle != script.get())
            CORE_WARN(u8"ScriptManager: Script and owner Entity don't match!");
        script_com.script_handle = nullptr;
        if(script) script->on_destroy();
    }
}
