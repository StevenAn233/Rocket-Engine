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
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        while(script_cache_.size() < storage.size()) script_cache_.emplace_back();
        for(Size index{}; index < storage.size(); index++)
        {
            NativeScriptComponent& nsc{ *(storage.begin() + index) };
            RuntimeCache& cache { *(script_cache_.begin() + index) };

            entt::entity owner_entity{ storage[index] };
            refresh_cache(cache, nsc.script_type,
                static_cast<EntityHandle>(owner_entity));
        }
    }

    void ScriptManager::on_runtime_stop()
    {
        for(auto& cache : script_cache_)
            destroy_script(std::move(cache.script));
        script_cache_.clear();
    }

    void ScriptManager::on_update(double dt)
    {
        if(!owner_->in_runtime()) return;
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        while(script_cache_.size() < storage.size()) script_cache_.emplace_back();
        for(Size index{}; index < storage.size(); index++)
        {
            NativeScriptComponent& nsc{ *(storage.begin() + index) };
            RuntimeCache& cache { *(script_cache_.begin() + index) };
            entt::entity owner_entity{ storage[index] };
            refresh_cache(cache, nsc.script_type,
                static_cast<EntityHandle>(owner_entity));
            if(cache.script) cache.script->on_update(dt);
        }
    }

    void ScriptManager::on_mouse_scrolled(float x_offset, float y_offset)
    {
        if(!owner_->in_runtime()) return;
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        while(script_cache_.size() < storage.size()) script_cache_.emplace_back();
        for(Size index{}; index < storage.size(); index++)
        {
            NativeScriptComponent& nsc{ *(storage.begin() + index) };
            RuntimeCache& cache { *(script_cache_.begin() + index) };
            entt::entity owner_entity{ storage[index] };
            refresh_cache(cache, nsc.script_type,
                static_cast<EntityHandle>(owner_entity));
            if(cache.script) cache.script->on_mouse_scrolled(x_offset, y_offset);
        }
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

    Scope<Script> ScriptManager::create_script(ScriptType type, EntityHandle owner)
    {
        if(type == script_type_null) return nullptr;

        auto& script_reg{ owner_->get_owner()->get_script_registry_mut() };
    // create script
        Scope<Script> script{ script_reg.construct_script(type) };
        if(!script) {
            CORE_ERROR(u8"ScriptManager: Failed to create script!");
            return nullptr;
        }
        script->owner_ = owner_->get_entity(owner);
        script->on_create();
        CORE_TRACE(u8"ScriptManager: Script '{}' created.",
            script_reg.get_script_name(type));
        return script;
    }

    void ScriptManager::destroy_script(Scope<Script> script)
    {
        if(!script) return;
        script->on_destroy();
        CORE_TRACE(u8"ScriptManager: Script destroyed.");
    }

    void ScriptManager::refresh_cache(RuntimeCache& cache, ScriptType type, EntityHandle owner)
    {
        if(type != cache.script_type)
        {
            cache.script_type = type;
            destroy_script(std::move(cache.script));
            cache.script = create_script(type, owner);
        }
    }

    void ScriptManager::contact_callback(EntityHandle owner_handle, EntityHandle other_handle, ContactType type)
    {
        Entity owner{ owner_->get_entity(owner_handle) };
        if(!owner.valid() || !owner.has<NativeScriptComponent>()) return;

        Entity other{ owner_->get_entity(other_handle) };
        if(!other.valid()) return;

        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        Size index{ storage.index(static_cast<entt::entity>(owner_handle)) };
        CORE_ASSERT(index < script_cache_.size(), u8"ScriptManager: Index out of bound!");

        RuntimeCache& cache{ *(script_cache_.begin() + index) };
        if(!cache.script) return;
        Script& script{ *(cache.script) };
        switch(type)
        {
        case ContactType::SolidBegin:  script.on_contact_solid_begin (other); break;
        case ContactType::SolidEnd:    script.on_contact_solid_end   (other); break;
        case ContactType::SensorBegin: script.on_contact_sensor_begin(other); break;
        case ContactType::SensorEnd:   script.on_contact_sensor_end  (other); break;
        default: break;
        }
    }

    void ScriptManager::on_script_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.script_manager, u8"ScriptManager: Null!");
        auto& script_cache{ ctx.script_manager->script_cache_ };
        if(script_cache.empty()) return;

        auto& storage{ reg.storage<NativeScriptComponent>() };
        Size index{ storage.index(ent) };
        CORE_ASSERT(index < script_cache.size(),
            u8"ScriptManager: Cache out of bound!");

        RuntimeCache& cache{ *(script_cache.begin() + index) };
        ctx.script_manager->destroy_script(std::move(cache.script));

        cache.script = std::move(script_cache.back().script);
        cache.script_type = script_cache.back().script_type;
        script_cache.pop_back();
    }
}
