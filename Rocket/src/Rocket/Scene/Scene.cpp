module;
module Scene;

import ScriptEngine;
import PhysicsEngine2D;
import ComponentRegistry;

namespace rke {
// Entity
    Entity::Entity(entt::entity handle, const Scene* scene)
        : handle_(handle), owner_scene_(scene) {}
    Entity::Entity(uint32 handle, const Scene* scene)
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

// Scene
    Scene::Scene(String name) : name_(std::move(name))
        { registry_ = create_scope<entt::registry>(); }
    Scene::~Scene() { clear(); }

    void Scene::on_attach()
    {
        // do something...
        CORE_INFO(u8"Scene: Scene '{}' attached.", name_);
    }

    void Scene::on_detach()
    {
        if(in_runtime_) on_runtime_stop();
        CORE_INFO(u8"Scene: Scene '{}' detached.", name_);
    }

    void Scene::clear()
    {
        registry_ ->clear();
        to_destroy_.clear();
        entity_map_.clear();
        gravity_ = {};
        master_cam_ = {};
        selected_entity_ = {};
    }

    std::vector<Entity> Scene::get_all_entities()
    {
        std::vector<Entity> entities{};
        auto all_entities{ registry_->view<TagComponent>() };
        for(auto entt : all_entities)
            entities.push_back({ entt, this });
        return entities;
    }

    Ref<Scene> Scene::deep_copy(bool temp)
    {
        Ref<Scene> new_scene{ create_ref<Scene>(name_) };
        new_scene->temporary_ = temp;

        new_scene->viewport_h_ = viewport_h_;
        new_scene->viewport_w_ = viewport_w_;

        const auto& storage{ registry_->storage<entt::entity>() };
        const auto* entities_data{ storage.data() };
        const Size count{ storage.size() };
        for(Size i{}; i < count; i++)
            (void)new_scene->registry_->create(entities_data[i]);
        // make sure orderly creation

        ComponentRegistry::each([&](auto type_id)
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
        if(master_cam.valid()) new_scene->master_cam_ =
            new_scene->get_entity(master_cam.get_uuid());
        // don't call set_camera_master() here cause it has already been copied

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
        { if(entity.valid()) to_destroy_.push_back(entity); }

    Entity Scene::copy_entity(Entity entity)
        { return copy_entity_towards(entity, this); }

    Entity Scene::copy_entity_towards(Entity entity, Scene* owner)
    {
        Entity copied_entity{ owner->registry_->create(), owner };
        mark_modified();

        copied_entity.emplace<UUIDComponent>(); // generate a unique one
        owner->entity_map_[copied_entity.get_uuid()] = copied_entity.handle_;

        ComponentRegistry::each([&](auto type_id)
        {
            using ComponentType = decltype(type_id)::Type;
            if constexpr(!std::is_same_v<ComponentType, UUIDComponent>)
                if(entity.has<ComponentType>()) copied_entity
                    .emplace_or_replace<ComponentType>(entity.get<ComponentType>());
        });

        return copied_entity;
    }

    void Scene::on_update(float dt)
    {
        if(in_runtime_) {
            ScriptEngine   ::on_update(dt);
            PhysicsEngine2D::on_update(dt);
        }
        flush_destroy_queue();
    }

    void Scene::on_runtime_start()
    {
        in_runtime_ = true;
        ScriptEngine   ::on_runtime_start(this);
        PhysicsEngine2D::on_runtime_start(this);
    }

    void Scene::on_runtime_stop()
    {
        in_runtime_ = false;
        ScriptEngine   ::on_runtime_stop();
        PhysicsEngine2D::on_runtime_stop();
    }

    void Scene::set_viewport(uint32 width, uint32 height)
    {
        viewport_w_ = width;
        viewport_h_ = height;
        refresh_camera_components();
    }

    void Scene::refresh_camera_components()
    {
        auto view{ registry_->view<CameraComponent>() };
        for(auto cam_entt : view) {
            auto& camera_com{ view.get<CameraComponent>(cam_entt) };
            if(!camera_com.aspect_ratio_fixed)
                camera_com.camera.set_viewport(viewport_w_, viewport_h_);
        }
    }

    void Scene::on_mouse_scrolled_runtime(MouseScrolledEvent& e)
        { if(in_runtime_) ScriptEngine::on_mouse_scrolled(e); }

    void Scene::set_camera_master(Entity entity)
    {
        if(!entity.valid()) {
            CORE_WARN(u8"Scene: Entity doesn't exist!");
            return;
        } else if(!entity.has<CameraComponent>()) {
            CORE_WARN(u8"Scene: Entity isn't a camera!");
            return;
        }
        if(entity.get_uuid() == master_cam_.get_uuid()) return;
        auto view{ registry_->view<CameraComponent>() };
        for(auto entt : view) view.get<CameraComponent>(entt).master = false;
        entity.get_mut<CameraComponent>().master = true;
        entity.get_mut<CameraComponent>().camera.set_viewport(viewport_w_, viewport_h_);
        master_cam_ = entity;
        CORE_INFO(u8"Scene: Master camera has been set to '{}'.",
            master_cam_.get<TagComponent>().tag);
        mark_modified();
    }

    Entity Scene::get_master_camera() const
    {
        if(master_cam_.valid()) return master_cam_;
        auto view{ registry_->view<CameraComponent>() };
        for(auto entt : view) {
            const auto& cam_com{ registry_->get<CameraComponent>(entt) };
            if(!cam_com.master) continue;
            
            master_cam_ = get_entity(entt);
            CORE_INFO(u8"Scene: Master camera has been set to '{}'.",
                master_cam_.get<TagComponent>().tag);
            return master_cam_;
        }
        return {};
    }

    void Scene::flush_destroy_queue()
    {
        if(to_destroy_.empty()) return;
        for(Entity entity : to_destroy_) {
            if(entity == master_cam_) master_cam_ = {};
            UUID uuid{ entity.get_uuid() };
            if(!uuid.empty()) entity_map_.erase(uuid);
            registry_->destroy(entity.handle_);
            mark_modified();
        }
        to_destroy_.clear();
    }
}
