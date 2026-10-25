module;
module ScriptManager;

import Log;
import Scene;
import Script;
import Project;
import ScriptRegistry;

namespace rke
{
    ScriptManager::ScriptManager(Scene* owner) : owner_(owner)
    {
        CORE_ASSERT(owner_, u8"ScriptManager: Owner scene empty!");
        owner_->registry_->on_destroy<NativeScriptComponent>()
            .connect<&on_script_com_destroy>();
        sync_all_to_cache();
    }

    ScriptManager::~ScriptManager() { clear(); }

    void ScriptManager::clear()
    {
        for(auto& cache : script_cache_)
            bury(std::move(cache.script));
        script_cache_.clear();
        flush_scripts();
    }

    void ScriptManager::on_update(double dt)
    {
        sync_all_to_cache();
        if(owner_->in_runtime())
            for_each_script([dt](Script* script){ script->on_update(dt); });
        flush_scripts();
    }

    void ScriptManager::on_mouse_scrolled(float x_offset, float y_offset)
    {
        sync_all_to_cache();
        if(owner_->in_runtime())
            for_each_script([x_offset, y_offset](Script* script)
                { script->on_mouse_scrolled(x_offset, y_offset); });
        flush_scripts();
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

    void ScriptManager::attach_script_to(Script* script, EntityHandle handle)
    {
        if(!script) return;
        if(!owner_->is_handle_valid(handle))
        {
            CORE_WARN(u8"ScriptManager: Owner handle invalid!");
            return;
        }
        script->owner_handle_ = handle;
        script->owner_scene_  = owner_;
    }

    void ScriptManager::init_script(Script* script, EntityHandle handle)
    {
        if(!script) return;
        attach_script_to(script, handle);
        script->on_create();
    }

    void ScriptManager::bury(Scope<Script> script)
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
            CORE_WARN(u8"ScriptManager: Script cache larger than storage"
                u8", which should't happen nomally!");
            bury(std::move(script_cache_.back().script));
            script_cache_.pop_back();
        }
    }

    ScriptManager::ScriptCache* ScriptManager::refresh_cache
        (EntityHandle handle, ScriptType type)
    {
        Size index{ owner_->registry_->storage<NativeScriptComponent>()
            .index(static_cast<entt::entity>(handle)) };
        if(index >= script_cache_.size())
        {
            CORE_ERROR(u8"ScriptManager: Index out of bound!");
            return nullptr;
        }

        ScriptCache& cache{ script_cache_[index] };
        if(!cache.script || cache.script_type != type) goto rebuild;
        if(cache.script->owner_handle_ != handle)
        {
            CORE_WARN(u8"ScriptManager: "
                u8"Script doesn't belong to this entity!");
            goto rebuild;
        }
        goto exit;
    rebuild:
        {
            bury(std::move(cache.script));

            const auto& script_reg{ owner_->get_owner()->get_script_registry() };
            Scope<Script> new_script{ script_reg.construct_script(type) };
            init_script(new_script.get(), handle);

            cache.script_type = type;
            cache.script = std::move(new_script);
        }
    exit:
        return &cache;
    }

    void ScriptManager::sync_all_to_cache()
    {
        align_cache();
        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        for(auto&& [ent, nsc] : storage.reach())
            refresh_cache(static_cast<EntityHandle>(ent), nsc.script_type);
    }

    void ScriptManager::flush_scripts() { graveyard_.clear(); }

    void ScriptManager::contact_callback
        (EntityHandle lhs, EntityHandle rhs, ContactType type)
    {
        if(!owner_->is_handle_valid(lhs) || !owner_->is_handle_valid(rhs)) return;
        if(!owner_->get_entity(lhs).has<NativeScriptComponent>()) return;

        Script* script{ get_script(lhs) };
        if(!script || script->owner_handle_ != lhs) return;
        switch(type)
        {
        case ContactType::SolidBegin:  script->on_contact_solid_begin (rhs); break;
        case ContactType::SolidEnd:    script->on_contact_solid_end   (rhs); break;
        case ContactType::SensorBegin: script->on_contact_sensor_begin(rhs); break;
        case ContactType::SensorEnd:   script->on_contact_sensor_end  (rhs); break;
        default: break;
        }
    }

    Script* ScriptManager::get_script(EntityHandle handle)
    {
        Entity entity{ owner_->get_entity(handle) };
        if(!entity.is_valid() || !entity.has<NativeScriptComponent>()) return nullptr;

        auto& storage{ owner_->registry_->storage<NativeScriptComponent>() };
        const Size index{ storage.index(static_cast<entt::entity>(handle)) };
        if(index >= script_cache_.size()) return nullptr;

        const ScriptCache& cache{ script_cache_.at(index) }; // do not refresh here
        ScriptType script_type{ entity.get<NativeScriptComponent>().script_type };
        if(cache.script_type != script_type/* outdated or invalid */) return nullptr;
        return cache.script.get();
    }

    void ScriptManager::on_script_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.script_manager, u8"ScriptManager: Null!");

        auto& script_cache{ ctx.script_manager->script_cache_ };
        if(script_cache.empty()) return;
        auto& storage{ reg.storage<NativeScriptComponent>() };
        if(script_cache.size() != storage.size())
        {
            CORE_WARN(u8"ScriptManager: Sizes not matching: "
                u8"cache {}, storage {}.", script_cache.size(), storage.size());
            return; // let it heal itself
        }
        CORE_ASSERT(storage.contains(ent),
            u8"ScriptManager: Entity doesn't has script component!");

    // swap and pop
        const Size index{ storage.index(ent) };
        ctx.script_manager->bury(std::move(script_cache[index].script));
        if(index + 1u != script_cache.size())
            script_cache[index] = std::move(script_cache.back());
        script_cache.pop_back();
    }
}
