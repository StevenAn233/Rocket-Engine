module;
module ScriptManager;

import Log;
import Scene;
import Project;
import Script;
import Components;
import ScriptRegistry;
import PhysicsEngine2D;

namespace rke
{
    ScriptManager::ScriptManager(Scene* owner) : owner_(owner)
    {
        CORE_ASSERT(owner_, u8"ScriptManager: Owner scene empty!");
        owner_->registry_->on_destroy<NativeScriptComponent>()
            .connect<&on_script_com_destroy>();
    }

    void ScriptManager::on_runtime_start()
    {
        auto& registry{ *(owner_->registry_) };
        auto view{ registry.view<NativeScriptComponent>() };
        for(entt::entity ent : view)
        {
            uint32 handle{ static_cast<uint32>(ent) };
            script_cache_[handle] = create_script(handle);
        }
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

    void ScriptManager::dispatch_contacts (
        const std::vector<Contact>& begin_contacts_solid,
        const std::vector<Contact>& end_contacts_solid,
        const std::vector<Contact>& begin_contacts_sensor,
        const std::vector<Contact>& end_contacts_sensor)
    {
        if(!owner_->in_runtime()) return;
        for(Contact contact : begin_contacts_solid)
        {
            contact_callback(contact.entity_a, contact.entity_b, ContactType::SolidBegin);
            contact_callback(contact.entity_b, contact.entity_a, ContactType::SolidBegin);
        }
        for(Contact contact : end_contacts_solid)
        {
            contact_callback(contact.entity_a, contact.entity_b, ContactType::SolidEnd);
            contact_callback(contact.entity_b, contact.entity_a, ContactType::SolidEnd);
        }
        for(Contact contact : begin_contacts_sensor)
            contact_callback(contact.entity_a, contact.entity_b, ContactType::SensorBegin);
        for(Contact contact : end_contacts_sensor)
            contact_callback(contact.entity_a, contact.entity_b, ContactType::SensorEnd);
    }

    void ScriptManager::contact_callback(uint32 owner_handle, uint32 other_handle, ContactType type)
    {
        auto it{ script_cache_.find(owner_handle) };
        if(it == script_cache_.end() || !it->second) return;

        Entity other{ owner_->get_entity(other_handle) };
        if(!other.valid()) return;

        switch(type)
        {
        case ContactType::SolidBegin:  it->second->on_contact_solid_begin (other); break;
        case ContactType::SolidEnd:    it->second->on_contact_solid_end   (other); break;
        case ContactType::SensorBegin: it->second->on_contact_sensor_begin(other); break;
        case ContactType::SensorEnd:   it->second->on_contact_sensor_end  (other); break;
        default: break;
        }
    }

    Scope<Script> ScriptManager::create_script(uint32 handle)
    {
    // check owner entity
        Entity entity{ owner_->get_entity(handle) };
        if(!entity.valid() || !entity.has<NativeScriptComponent>())
            { CORE_ERROR(u8"ScriptManager: Entity not valid!"); return nullptr; }
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
    // set mutual refs
        script_com.script_handle = script.get();
        script->owner_ = entity;

        script->on_create();
        return script;
    }

    void ScriptManager::destroy_script(Scope<Script> script, uint32 handle)
    {
        if(script) script->on_destroy();
    // check owner entity
        Entity entity{ owner_->get_entity(handle) };
        if(!entity.valid() || !entity.has<NativeScriptComponent>())
            { CORE_ERROR(u8"ScriptManager: Entity not valid!"); return; }
        auto& script_com{ entity.get_mut<NativeScriptComponent>() };
        if(script_com.script_handle != script.get())
            CORE_WARN(u8"ScriptManager: Script and owner Entity don't match!");
        script_com.script_handle = nullptr;
    }

    void ScriptManager::on_script_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.script_manager, u8"ScriptManager: Null!");
        auto& script_cache{ ctx.script_manager->script_cache_ };
        if(script_cache.empty() ||
          !script_cache.contains(static_cast<uint32>(ent))) return;

        Scope<Script> script{ std::move(script_cache.at(static_cast<uint32>(ent))) };
        script->on_destroy();
        script_cache.erase(static_cast<uint32>(ent));
        reg.get<NativeScriptComponent>(ent).script_handle = nullptr;
    }
}
