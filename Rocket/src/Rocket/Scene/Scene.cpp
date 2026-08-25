module;
module Scene;

import Log;
import Components;
import Project;
import PhysicsEngine2D;
import ScriptManager;

namespace rke
{
    Entity::Entity() : handle_(entt::null), owner_scene_(nullptr) {}

    Entity::Entity(entt::entity handle, Scene* scene)
        : handle_(handle), owner_scene_(scene) {}
    
    Entity::Entity(uint32 handle, Scene* scene)
        : handle_(entt::entity(handle)), owner_scene_(scene) {}

    bool Entity::valid() const
    {
        if(empty()) return false;
        if(!owner_scene_) return false;
        if(!owner_scene_->registry_->valid(handle_)) return false;
        return true;
    }

    void Entity::invalidate_if_unavailable()
    {
        if(!valid()) {
            handle_ = entt::null;
            owner_scene_ = nullptr;
        }
    }

    bool Entity::operator==(const Entity& other) const
    {
        return handle_ == other.handle_ &&
          owner_scene_ == other.owner_scene_;
    }

    bool Entity::operator!=(const Entity& other) const { return !operator==(other); }

    void Entity::refresh_script()
    {
        if(!owner_scene_) return;
        owner_scene_->refresh_script(*this);
    }

    UUID Entity::get_uuid() const
    {
        if(!valid()) return UUID(0);
        return get<UUIDComponent>().uuid;
    }

    void Entity::check_assert() const { CORE_ASSERT(valid(), u8"Entity: Invalid!"); }

    Scene::Scene(Project* owner, String name)
        : owner_(owner), name_(std::move(name))
    {
        registry_ = create_scope<entt::registry>();
        script_manager_ = create_scope<ScriptManager>(this);
        physics_engine_ = PhysicsEngine2D::create(this);

        registry_->ctx().emplace<RegistryContext>
            (script_manager_.get(), physics_engine_.get());
    }

    Scene::~Scene() { if(in_runtime_) on_runtime_stop(); clear(); }

    void Scene::set_name(String name)
    {
        if(name.empty()) name_ = u8"Untitled";
        else name_ = std::move(name);
        mark_modified();
    }

    Path Scene::get_path() const { return owner_->get_scenes_dir() / (name_ + u8".rkscene"); }

    Scope<Scene> Scene::deep_copy(bool temp)
    {
        Scope<Scene> new_scene{ create_scope<Scene>(owner_, name_) };
        new_scene->temporary_ = temp;

        new_scene->viewport_h_ = viewport_h_;
        new_scene->viewport_w_ = viewport_w_;

        const auto& storage{ registry_->storage<entt::entity>() };
        const auto* entities_data{ storage.data() };
        const Size count{ storage.size() };
        for(Size i{}; i < count; i++)
            (void)new_scene->registry_->create(entities_data[i]);
        // make sure orderly creation

        components::each([&](auto type_id)
        {
            using ComponentType = decltype(type_id)::Type;
            auto view{ registry_->view<ComponentType>() };
            for(auto it{ view.rbegin() }; it != view.rend(); ++it)
            {
                entt::entity src_entt{ *it };
                const auto& src_com{ registry_->get<ComponentType>(src_entt) };
                new_scene->registry_->
                    emplace_or_replace<ComponentType>(src_entt, src_com);
            }
        });

        new_scene->gravity_ = gravity_;

        // after UUIDComponents are copied
        auto uuid_view{ new_scene->registry_->view<UUIDComponent>() };
        for(auto entt : uuid_view)
        {
            UUID uuid{ new_scene->registry_->get<UUIDComponent>(entt).uuid };
            new_scene->entity_map_[uuid] = entt;
        }
        Entity master_cam{ get_master_camera() }; // refresh
        if(master_cam.valid())
            new_scene->master_cam_ = new_scene->get_entity(master_cam.get_uuid());
        // don't call set_camera_master() here cause it has already been copied

        new_scene->selected_entity_ = {};
        return new_scene;
    }

    Entity Scene::create_entity(String tag, UUID uuid)
    {
        Entity entity(registry_->create(), this);
        mark_modified();

        entity.emplace<UUIDComponent>(uuid);
        if(!uuid.empty()) entity_map_[entity.get_uuid()] = entity.handle_;

        entity.emplace<TagComponent>(std::move(tag));
        entity.emplace<TransformComponent>();
        return entity;
    }

    void Scene::destroy_entity(Entity entity)
    {
        if(entity.empty()) return;
        if(!entity.belongs_to(this)) {
            CORE_ERROR(u8"Scene: Entity doesn't belong to this scene!");
            return;
        }
        if(entity == selected_entity_) set_selected_entity(Entity{});
        if(entity == master_cam_) master_cam_ = {};
        if(entity == demo_cam_) demo_cam_ = {};
        to_destroy_.push_back(entity);
    }

    std::vector<Entity> Scene::get_all_entities()
    {
        std::vector<Entity> entities{};
        auto all_entities{ registry_->view<TagComponent>() };
        for(auto entt : all_entities)
            entities.push_back({ entt, this });
        return entities;
    }

    bool Scene::has_entity(UUID uuid) const
    {
        if(uuid.empty()) return false;
        return entity_map_.find(uuid) != entity_map_.end();
    }

    Entity Scene::get_entity(uint32 handle)
    {
        entt::entity entt{ static_cast<entt::entity>(handle) };
        if(registry_->valid(entt)) return Entity(entt, this);
        return {};
    }

    Entity Scene::get_entity(UUID uuid)
    {
        if(uuid.empty()) return {};
        if(has_entity(uuid)) return Entity(entity_map_.at(uuid), this);
        CORE_ERROR(u8"Scene: Entity UUID '{}' not found!", uuid.value());
        return {};
    }

    const Entity Scene::get_entity(uint32 handle) const
    {
        entt::entity entt{ static_cast<entt::entity>(handle) };
        if(registry_->valid(entt)) return Entity(entt, const_cast<Scene*>(this));
        return {};
    }

    const Entity Scene::get_entity(UUID uuid) const
    {
        if(uuid.empty()) return {};
        if(has_entity(uuid)) return Entity(entity_map_.at(uuid), const_cast<Scene*>(this));
        CORE_ERROR(u8"Scene: Entity UUID '{}' not found!", uuid.value());
        return {};
    }

    Entity Scene::copy_entity_towards(Entity entity, Scene* owner)
    {
        if(!entity.belongs_to(this))
            CORE_WARN(u8"Scene: Entity doesn't belong to this scene!");
        Entity copied_entity{ owner->registry_->create(), owner };
        mark_modified();

        copied_entity.emplace<UUIDComponent>(); // generate a unique one
        owner->entity_map_[copied_entity.get_uuid()] = copied_entity.handle_;

        components::each([&](auto type_id)
        {
            using ComponentType = decltype(type_id)::Type;
            if constexpr(!std::is_same_v<ComponentType, UUIDComponent>)
                if(entity.has<ComponentType>()) copied_entity
                    .emplace_or_replace<ComponentType>(entity.get<ComponentType>());
        });

        return copied_entity;
    }

    void Scene::set_selected_entity(Entity entity)
    {
        if(entity.empty())
        {
            selected_entity_ = {};
            return;
        }
        if(!entity.valid() || !entity.belongs_to(this)) {
            CORE_ERROR(u8"Scene: Entity doesn't belong to this scene!");
            return;
        }
        selected_entity_ = entity;
        if(entity.has<CameraComponent>()) set_demo_camera(entity);
    }

    void Scene::set_master_camera(Entity entity)
    {
        if(entity == master_cam_) return;
        if(!entity.valid() || !entity.belongs_to(this))
        {
            CORE_ERROR(u8"Scene: Entity invalid!");
            return;
        }
        if(!entity.has<CameraComponent>()) {
            CORE_ERROR(u8"Scene: Entity isn't a camera!");
            return;
        }
        master_cam_ = entity;
        mark_modified();
    }

    void Scene::set_demo_camera(Entity entity)
    {
        if(entity == demo_cam_) return;
        if(!entity.valid() || !entity.belongs_to(this))
        {
            CORE_ERROR(u8"Scene: Entity invalid!");
            return;
        }
        if(!entity.has<CameraComponent>())
        {
            CORE_ERROR(u8"Scene: Entity isn't a camera!");
            return;
        }
        demo_cam_ = entity;
    }

    void Scene::refresh_script(Entity entity)
    {
        if(!entity.valid() || !entity.belongs_to(this)) return;
        script_manager_->refresh_script(entity.get_handle());
    }

    void Scene::clear()
    {
        if(in_runtime()) {
            CORE_ERROR(u8"Scene: Can't be cleared while in runtime!");
            return;
        }
        registry_ ->clear();
        to_destroy_.clear();
        entity_map_.clear();
        gravity_ = {};
        demo_cam_ = {};
        master_cam_ = {};
        selected_entity_ = {};
    }

    void Scene::on_update(float dt)
    {
        if(in_runtime())
        {
            auto view{ registry_->view<NativeScriptComponent>() };
            for(entt::entity ent : view)
            {
                Script* script{ registry_->get<NativeScriptComponent>(ent).script_handle };
                if(script) script->on_update(dt);
            }
            physics_engine_->on_update(dt);
        }
        flush_destroy_queue();
    }

    void Scene::on_runtime_start()
    {
        CORE_ASSERT(!in_runtime(), u8"Scene: Already in runtime!");
        in_runtime_ = true;
        script_manager_->on_runtime_start();
        physics_engine_->on_runtime_start();
    }

    void Scene::on_runtime_stop()
    {
        CORE_ASSERT(in_runtime(), u8"Scene: Not in runtime!");
        in_runtime_ = false;
        script_manager_->on_runtime_stop();
        physics_engine_->on_runtime_stop();
    }

    void Scene::set_viewport(uint32 width, uint32 height)
    {
        viewport_w_ = width;
        viewport_h_ = height;
        auto view{ registry_->view<CameraComponent>() };
        for(auto cam_entt : view)
        {
            auto& camera_com{ view.get<CameraComponent>(cam_entt) };
            if(!camera_com.aspect_ratio_fixed)
                camera_com.camera.set_viewport(viewport_w_, viewport_h_);
        }
    }

    void Scene::on_mouse_scrolled_runtime(MouseScrolledEvent& e)
    {
        if(!in_runtime()) return;
        auto view{ registry_->view<NativeScriptComponent>() };
        for(entt::entity ent : view)
        {
            Script* script{ registry_->get<NativeScriptComponent>(ent).script_handle };
            if(script) script->on_mouse_scrolled(e.get_x_offset(), e.get_y_offset());
        }
    }

    void Scene::flush_destroy_queue()
    {
        while(!to_destroy_.empty())
        {
            Entity curr{ to_destroy_.back() };
            to_destroy_.pop_back();
            if(!curr.valid()) continue;

            UUID uuid{ curr.get_uuid() };
            if(!uuid.empty()) entity_map_.erase(uuid);

            registry_->destroy(curr.handle_);
            mark_modified();
        }
    }
}
