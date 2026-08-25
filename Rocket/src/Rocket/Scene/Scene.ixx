module;

#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "rke_macros.h"
namespace rke { class Project; class ScriptManager; }

export module Scene;

import UUID;
import Types;
import String;
import Path;
import MouseEvent;
import ApplicationEvent;
import HeapManager;
import PhysicsLayers;
import Gravity2D;
import PhysicsEngine2D;

export namespace rke
{
    class Entity
    {
    public:
        friend class Scene;
        friend class SceneRenderer;

        RKE_API Entity();

        RKE_API Entity(const Entity&) = default;
        RKE_API Entity& operator=(const Entity&) = default;
        RKE_API Entity(Entity&&) = default;
        RKE_API Entity& operator=(Entity&&) = default;

        RKE_API UUID get_uuid() const;
        RKE_API bool valid() const;
        RKE_API void invalidate_if_unavailable();

        RKE_API bool operator==(const Entity& other) const;
        RKE_API bool operator!=(const Entity& other) const;

        inline Scene* get_owner() { return owner_scene_; } // mutable
        RKE_API void refresh_script();

        inline uint32 get_handle() const { return static_cast<uint32>(handle_); }
        inline bool empty() const { return handle_ == entt::null; }
        inline bool belongs_to(const Scene* scene) const { return scene == owner_scene_; }

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
    private:
        RKE_API Entity(entt::entity handle, Scene* scene);
        RKE_API Entity(uint32 handle, Scene* scene);

        RKE_API void check_assert() const;
    private:
        entt::entity handle_; // version(12bits) + index(20bits)
        Scene* owner_scene_;
    };

    constexpr uint32 entity_id_null{ 0xFFFFFFFFu };

    class RKE_API Scene
    {
    public:
        friend class Entity;
        friend class SceneSerializer;
        friend class ScriptManager;
        friend class PhysicsEngine2D;
        friend class SceneRenderer;

        struct RegistryContext
        {
            ScriptManager* script_manager{};
            PhysicsEngine2D* physics_engine{};
        };

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

        Entity get_entity(uint32 handle);
        Entity get_entity(UUID uuid);
        const Entity get_entity(uint32 handle) const;
        const Entity get_entity(UUID uuid) const;

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

        void refresh_script(Entity entity); // ScriptManager::refresh_script

        void clear();
        void on_update(double dt);

        void on_runtime_start();
        void on_runtime_stop ();

        void on_mouse_scrolled_runtime(MouseScrolledEvent& e);

        void set_viewport(uint32 width, uint32 height);
        inline uint32 get_viewport_w() const { return viewport_w_; }
        inline uint32 get_viewport_h() const { return viewport_h_; }

        inline bool in_runtime() const { return in_runtime_; }
        inline bool to_save() const { return !temporary_ && modified_; }

    // or these thing should be implemented together with Undoing?
        inline void mark_modified() const { modified_ = true; }
        inline void mark_modified_if(bool condition) const { if(condition) modified_ = true; }

        inline glm::vec2 get_gravity() const { return gravity_.get(); }
        inline glm::vec2& get_gravity_mut() { return gravity_.get_mut(); }
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

        Scope<ScriptManager> script_manager_{};
        Scope<PhysicsEngine2D> physics_engine_{};
    };

    template<typename Component>
    bool Entity::has() const
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        return owner_scene_->registry_->all_of<Component>(handle_);
    }

    template<typename ...Components>
    bool Entity::has_all_of() const
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        return owner_scene_->registry_->all_of<Components...>(handle_);
    }

    template<typename ...Components>
    bool Entity::has_any_of() const
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        return owner_scene_->registry_->any_of<Components...>(handle_);
    }

    template<typename Component, typename ...Args>
    Component& Entity::emplace(Args&& ...args)
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        owner_scene_->mark_modified();
        return owner_scene_->registry_->emplace<Component>
            (handle_, std::forward<Args>(args)...);
    }

    template<typename Component, typename ...Args>
    Component& Entity::emplace_or_replace(Args&& ...args)
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        owner_scene_->mark_modified();
        return owner_scene_->registry_->emplace_or_replace<Component>
            (handle_, std::forward<Args>(args)...);
    }

    template<typename Component>
    const Component& Entity::get() const
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        return owner_scene_->registry_->get<Component>(handle_);
    }

    template<typename Component>
    Component& Entity::get_mut()
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        return owner_scene_->registry_->get<Component>(handle_);
    }

    template<typename Component>
    void Entity::remove()
    {
    #ifdef RKE_DEBUG
        check_assert();
    #endif
        owner_scene_->registry_->remove<Component>(handle_);
        owner_scene_->mark_modified();
    }
}
