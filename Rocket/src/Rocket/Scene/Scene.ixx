module;

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "rke_macros.h"
namespace rke { class Project; }

export module Scene;

import Log;
import UUID;
import Types;
import String;
import Path;
import Event;
import MouseEvent;
import ApplicationEvent;
import HeapManager;
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
        entt::entity handle_{ entt::null }; // version(12bits) + index(20bits)
        const Scene* owner_scene_{ nullptr };
    };

    constexpr uint32 entity_id_null{ 0xFFFFFFFFu };

    class RKE_API Scene
    {
    public:
        friend class Entity;
        friend class SceneSerializer;
        friend class ScriptEngine;
        friend class PhysicsEngine2D;
        friend class SceneRenderer;

        Scene(Project* owner, String name = u8"Untitled");
        ~Scene();

        Scene(const Scene&) = delete;
        Scene(Scene&&) = delete;
        
        void set_name(String name);
        inline const String& get_name() const { return name_; }
        
        inline Project* get_owner() const { return owner_; }
        Path get_path() const;

        Scope<Scene> deep_copy(bool temp = true);

        Entity create_entity(String tag = u8"New Entity", UUID uuid = {});
        void destroy_entity(Entity entity);
        void destroy_entity(uint32 handle) { destroy_entity(get_entity(handle)); }
        void destroy_entity(UUID uuid) { destroy_entity(get_entity(uuid)); }

        std::vector<Entity> get_all_entities();
        bool has_entity(UUID uuid) const;

        Entity copy_entity(Entity entity) { return copy_entity_towards(entity, this); }
        Entity copy_entity_towards(Entity entity, Scene* owner);

        Entity get_entity(uint32 handle) const;
        Entity get_entity(UUID uuid) const;

        Entity get_selected_entity() const { return selected_entity_; }
        void destroy_selected_entity() { destroy_entity(selected_entity_); }
        void set_selected_entity(Entity entity);
        void set_selected_entity(uint32 handle) { set_selected_entity(get_entity(handle)); }
        void set_selected_entity(UUID uuid) { set_selected_entity(get_entity(uuid)); }
        
        Entity get_master_camera() const { return master_cam_; }
        void set_master_camera(Entity entity);
        void set_master_camera(uint32 handle) { set_master_camera(get_entity(handle)); }
        void set_master_camera(UUID uuid) { set_master_camera(get_entity(uuid)); }

        Entity get_demo_camera() const { return demo_cam_; }
        void set_demo_camera(Entity entity);
        void set_demo_camera(uint32 handle) { set_demo_camera(get_entity(handle)); }
        void set_demo_camera(UUID uuid) { set_demo_camera(get_entity(uuid)); }

        void clear();
        void on_update(float dt);

        void on_runtime_start();
        void on_runtime_stop ();

        void on_mouse_scrolled_runtime(MouseScrolledEvent& e);

        void set_viewport(uint32 width, uint32 height);
        inline uint32 get_viewport_w() const { return viewport_w_; }
        inline uint32 get_viewport_h() const { return viewport_h_; }

        bool in_runtime() const { return in_runtime_; }
        bool to_save() const { return !temporary_ && modified_; }

    // or these thing should be implemented together with Undoing?
        void mark_modified() const { if(!temporary_) modified_ = true; }
        void mark_modified_if(bool condition) const
            { if(!temporary_ && condition) modified_ = true; }

        glm::vec2  get_gravity() const { return gravity_.get(); }
        glm::vec2& get_gravity_mut() { return gravity_.get_mut(); }

        static void set_on_entity_selected(std::function<void(Entity)> callback)
            { on_entity_selected_ = std::move(callback); }
    private:
        void flush_destroy_queue();
    private:
        Project* owner_;
        String name_;

        Scope<entt::registry> registry_{};
        std::vector<Entity> to_destroy_{};

        uint32 viewport_w_{}, viewport_h_{};
        Gravity2D gravity_{};
        bool in_runtime_{ false };

        mutable bool modified_{ false };
        bool temporary_{ false }; // not gonna serialize

        std::unordered_map<UUID, entt::entity> entity_map_{};
        Entity master_cam_{};
        Entity demo_cam_{};
        Entity selected_entity_{}; // std::vector<Entity> selected_entities{};

        static std::function<void(Entity)> on_entity_selected_;
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
