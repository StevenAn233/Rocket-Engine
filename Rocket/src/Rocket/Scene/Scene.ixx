module;

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Scene;

import Types;
import Log;
import String;
import Event;
import Texture;
import HeapManager;
import UUID;
import Components;
import PhysicsLayers;
import Gravity2D;

export namespace rke
{
    class Entity
    {
    public:
        friend class Scene;
        friend class SceneRenderer;

        RKE_API Entity() = default;
        RKE_API Entity(const Entity&) = default;
        RKE_API Entity& operator=(const Entity&) = default;
        RKE_API Entity(Entity&&) = default;
        RKE_API Entity& operator=(Entity&&) = default;

        RKE_API UUID get_uuid() const
        {
            if(!valid()) return UUID(0);
            return get<UUIDComponent>().uuid;
        }

        template<typename Component>
        bool has() const;
        template<typename ...Components>
        bool has_all_of() const;
        template<typename ...Components>
        bool has_any_of() const;

        template<typename Component, typename ...Args>
        Component& emplace(Args&&... args);
        template<typename Component, typename ...Args>
        Component& emplace_or_replace(Args&&... args);
        template<typename Component>
        const Component& get() const;
        template<typename Component>
        Component& get_mut();
        template<typename Component>
        void remove();

        RKE_API uint32 get_handle() const { return static_cast<uint32>(handle_); }
        RKE_API bool empty() const { return handle_ == entt::null; }
        RKE_API bool valid() const;
        RKE_API void invalidate_if_unavailable();
        RKE_API bool belongs_to(Scene* scene) const { return scene == owner_scene_; }

        RKE_API bool operator==(const Entity& other) const;
        RKE_API bool operator!=(const Entity& other) const;
        RKE_API bool is_owner(const Scene* scene) const { return owner_scene_ == scene; }
    private:
        RKE_API Entity(entt::entity handle, const Scene* scene);
        RKE_API Entity(uint32 handle, const Scene* scene);
    private:
        entt::entity handle_{ entt::null };
        const Scene* owner_scene_{ nullptr }; // doesn't own the scene
    };

    class RKE_API Scene
    {
    public:
        friend class Entity;
        friend class SceneSerializer;
        friend class ScriptEngine;
        friend class PhysicsEngine2D;
        friend class SceneRenderer;

        explicit Scene(String name = u8"Untitled");
        Scene(const Scene&) = delete;
        Scene(Scene&& ____) = delete;
        ~Scene();

        void on_attach(); // Called by Project
        void on_detach(); // Called by Project
        void clear();

        const String& get_name() const { return name_; }
        void set_name(String name)
        {
            if(name.empty()) name_ = u8"Untitled";
            else name_ = std::move(name);
            mark_modified();
        }

        std::vector<Entity> get_all_entities();
        Ref<Scene> deep_copy(bool temp = true);

        Entity create_entity(String tag = u8"New Entity", UUID uuid = {});
        Entity get_entity(entt::entity handle, bool warn = true) const
        {
            if(registry_->valid(handle)) return Entity(handle, this);
            if(warn) CORE_WARN(u8"Scene: Entity handle is not valid!");
            return {};
        }
        Entity get_entity(uint32 handle, bool warn = true) const
            { return get_entity(static_cast<entt::entity>(handle), warn); }
        Entity get_entity(int probable, bool warn = true) const
        {
            if(probable <= -1) {
                if(warn) CORE_WARN(u8"Scene: Entity handle is null!");
                return {};
            }
            return get_entity(static_cast<uint32>(probable), warn);
        }

        bool has_entity(UUID uuid) const
            { return (entity_map_.find(uuid) != entity_map_.end()); }
        Entity get_entity(UUID uuid) const
        {
            if(uuid.empty()) return {};
            if(entity_map_.find(uuid) != entity_map_.end())
                return { entity_map_.at(uuid), this };
            CORE_ERROR(u8"Scene: Entity UUID '{}' not found!", uuid.value());
            return {};
        }

        void destroy_entity(Entity entity);
        Entity  copy_entity(Entity entity);
        Entity  copy_entity_towards(Entity entity, Scene* owner);

        void on_update(float dt);

        void on_runtime_start();
        void on_runtime_stop ();

        void on_mouse_scrolled_runtime(MouseScrolledEvent& e);
        void on_viewport_resized_runtime(ViewportResizedEvent& e)
            { set_viewport(e.get_width(), e.get_height()); }

        void set_camera_master(Entity entity);
        void set_viewport(uint32 width, uint32 height);
        void refresh_camera_components();

        bool in_runtime() const { return in_runtime_; }
        bool to_save() const { return !temporary_ && modified_; }

    // or these thing should be implemented together with Undoing?
        void mark_modified() const { if(!temporary_) modified_ = true; }
        void mark_modified_if(bool condition) const
            { if(!temporary_ && condition) modified_ = true; }
    // ---

        Entity get_master_camera() const;
        glm::vec2  get_gravity() const { return gravity_.get(); }
        glm::vec2& get_gravity_mut() { return gravity_.get_mut(); }

        Entity get_selected_entity() const { return selected_entity_; }
        void set_selected_entity(uint32 handle)
        {
            selected_entity_ = get_entity(handle);
            if(on_entity_selected_) on_entity_selected_(selected_entity_);
        }
        void set_selected_entity(int propable)
        {
            selected_entity_ = get_entity(propable, false);
            if(on_entity_selected_) on_entity_selected_(selected_entity_);
        }
        void set_selected_entity(UUID uuid)
        {
            selected_entity_ = get_entity(uuid);
            if(on_entity_selected_) on_entity_selected_(selected_entity_);
        }
        void set_selected_entity(Entity entity)
        {
            if(entity.empty() || (entity.valid() && entity.belongs_to(this)))
            {
                selected_entity_ = entity;
                if(on_entity_selected_) on_entity_selected_(selected_entity_);
            }
            else CORE_ERROR(u8"Scene: Selected entity invalid!");
        }
        void destroy_selected_entity()
        {
            if(selected_entity_.valid())
                destroy_entity(selected_entity_);
            set_selected_entity(Entity{});
        }
        Entity copy_selected_entity() { return copy_entity(selected_entity_); }

        static void set_on_entity_selected(std::function<void(Entity)> callback)
            { on_entity_selected_ = std::move(callback); }
    private:
        void flush_destroy_queue();
    private:
        String name_;

        Scope<entt::registry> registry_{};
        std::vector<Entity> to_destroy_{};

        uint32 viewport_w_{}, viewport_h_{};
        Gravity2D gravity_{};
        bool in_runtime_{ false };

    // for mark_modified()
        mutable bool modified_{ false };
        bool temporary_{ false };
    // ---

        mutable Entity master_cam_{};
        std::unordered_map<UUID, entt::entity> entity_map_{};

        Entity selected_entity_{}; // std::vector<Entity> selected_entities{};
        static inline std::function<void(Entity)> on_entity_selected_{};
    };

    template<typename Component>
    bool Entity::has() const
    {
        CORE_ASSERT(valid(), u8"Entity: Invalid!");
        return owner_scene_->registry_->all_of<Component>(handle_);
    }

    template<typename ...Components>
    bool Entity::has_all_of() const
    {
        CORE_ASSERT(valid(), u8"Entity: Invalid!");
        return owner_scene_->registry_->all_of<Components...>(handle_);
    }

    template<typename ...Components>
    bool Entity::has_any_of() const
    {
        CORE_ASSERT(valid(), u8"Entity: Invalid!");
        return owner_scene_->registry_->any_of<Components...>(handle_);
    }

    template<typename Component, typename ...Args>
    Component& Entity::emplace(Args&& ...args)
    {
        CORE_ASSERT(!has<Component>(), u8"Entity: Try to emplace the same component!");
        owner_scene_->mark_modified();
        return owner_scene_->registry_->emplace<Component>
            (handle_, std::forward<Args>(args)...);
    }

    template<typename Component, typename ...Args>
    Component& Entity::emplace_or_replace(Args && ...args)
    {
        CORE_ASSERT(!has<Component>(), u8"Entity: Try to emplace the same component!");
        owner_scene_->mark_modified();
        return owner_scene_->registry_->emplace_or_replace<Component>
            (handle_, std::forward<Args>(args)...);
    }

    template<typename Component>
    const Component& Entity::get() const
    {
        CORE_ASSERT(has<Component>(), u8"Entity: Try to get non-existed component!");
        return owner_scene_->registry_->get<Component>(handle_);
    }

    template<typename Component>
    Component& Entity::get_mut()
    {
        CORE_ASSERT(has<Component>(), u8"Entity: Try to get non-existed component!");
        return owner_scene_->registry_->get<Component>(handle_);
    }

    template<typename Component>
    void Entity::remove()
    {
        CORE_ASSERT(has<Component>(), u8"Entity: Try to get non-existed component!");
        owner_scene_->registry_->remove<Component>(handle_);
        owner_scene_->mark_modified();
    }
}
