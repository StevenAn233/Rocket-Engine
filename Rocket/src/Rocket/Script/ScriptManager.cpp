module;
module ScriptManager;

import Log;
import Scene;
import Project;
import ScriptRegistry;

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
        align_cache();
        Size index{};
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        for(auto&& [ent, nsc] : storage.reach())
        {
        #ifdef RKE_DEBUG
            CORE_ASSERT(index == storage.index(ent),
                u8"ScriptManager: Indices are not matching: {}, {}!",
                index, storage.index(ent));
        #endif
            refresh_cache(static_cast<EntityHandle>(ent), nsc, index++);
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
        align_cache();
        Size index{}; // should be the same with storage.index(entity)
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        for(auto&& [ent, nsc] : storage.reach())
        {
        #ifdef RKE_DEBUG
            CORE_ASSERT(index == storage.index(ent),
                u8"ScriptManager: Indices are not matching: {}, {}!",
                index, storage.index(ent));
        #endif
            RuntimeCache* cache{ refresh_cache
                (static_cast<EntityHandle>(ent), nsc, index++) };
            if(cache && cache->script) cache->script->on_update(dt);
        }
        flush_scripts();
    }

    void ScriptManager::on_mouse_scrolled(float x_offset, float y_offset)
    {
        if(!owner_->in_runtime()) return;
        align_cache();
        Size index{};
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        for(auto&& [ent, nsc] : storage.reach())
        {
        #ifdef RKE_DEBUG
            CORE_ASSERT(index == storage.index(ent),
                u8"ScriptManager: Indices are not matching: {}, {}!",
                index, storage.index(ent));
        #endif
            RuntimeCache* cache{ refresh_cache
                (static_cast<EntityHandle>(ent), nsc, index++) };
            if(cache && cache->script) cache->script->on_mouse_scrolled(x_offset, y_offset);
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
        return script;
    }

    void ScriptManager::destroy_script(Scope<Script> script)
    {
        if(!script) return;
        script->on_destroy();
        graveyard_.push_back(std::move(script));
    }

    void ScriptManager::align_cache()
    {
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        while(script_cache_.size() < storage.size()) script_cache_.emplace_back();
        while(script_cache_.size() > storage.size())
        {
            destroy_script(std::move(script_cache_.back().script));
            script_cache_.pop_back();
        }
    }

    ScriptManager::RuntimeCache* ScriptManager::refresh_cache
        (EntityHandle handle, NativeScriptComponent& nsc, Size index)
    {
        const entt::entity ent{ static_cast<entt::entity>(handle) };
        CORE_ASSERT(index < script_cache_.size(),
            u8"ScriptManager: Index out of bound!");
        RuntimeCache& cache{ script_cache_[index] };

        if(cache.owner != handle || cache.script_type != nsc.script_type)
        {
            destroy_script(std::move(cache.script));
            cache.owner       = handle;
            cache.script_type = nsc.script_type;
            cache.script      = create_script(nsc.script_type, handle);
        }
        return &cache;
    }

    void ScriptManager::flush_scripts() { graveyard_.clear(); }

    void ScriptManager::contact_callback
        (EntityHandle owner_handle, EntityHandle other_handle, ContactType type)
    {
        Entity owner{ owner_->get_entity(owner_handle) };
        if(!owner.valid() || !owner.has<NativeScriptComponent>()) return;

        Entity other{ owner_->get_entity(other_handle) };
        if(!other.valid()) return;

        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        entt::entity ent{ static_cast<entt::entity>(owner_handle) };
        const Size index{ storage.index(ent) };
        auto& nsc{ storage.get(ent) };

        RuntimeCache* cache{ refresh_cache(owner_handle, nsc, index) };
        if(!cache || !cache->script) return;
        Script& script{ *(cache->script) };
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
        auto& storage{ reg.storage<NativeScriptComponent>() };
    // fires *before* EnTT pops the element: mirror the upcoming swap-and-pop
    // while both arrays are in sync, otherwise let the per-frame identity
    // check of refresh_cache() repair the cache on its own.
        if(script_cache.size() != storage.size()) return;
        CORE_ASSERT(storage.contains(ent),
            u8"ScriptManager: Entity doesn't has script component!");

    // swap and pop
        const Size index{ storage.index(ent) };
        ctx.script_manager->destroy_script(std::move(script_cache[index].script));
        if(index + 1u != script_cache.size())
            script_cache[index] = std::move(script_cache.back());
        script_cache.pop_back();
    }
}
