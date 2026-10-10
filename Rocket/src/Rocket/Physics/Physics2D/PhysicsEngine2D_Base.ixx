module;

#include <vector>
#include <unordered_map>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
namespace rke { class Scene; class Entity; }

export module PhysicsEngine2D:Base;

import Types;
import Plane;
import Gravity;
import EntityAccess;
import HeapManager;

export namespace rke
{
    class PhysicsEngine2D
    {
    public:
        PhysicsEngine2D(Scene* scene);
        virtual ~PhysicsEngine2D() = default;

        virtual void on_runtime_start() = 0;
        virtual void on_runtime_stop () = 0;

        virtual void on_update(double dt) = 0;
        virtual bool empty() const = 0;
        virtual void apply_force(EntityHandle entity, glm::vec2 force) = 0;

        inline void set_gravity(glm::vec3 val) { gravity_.ref() = val; }

        inline void set_plane(glm::vec4 plane)
            { plane_ = PlaneBasis(plane); plane_dirty_ = true; }
        inline void set_plane(glm::vec3 axis) { set_plane(glm::vec4(axis, 0.0f)); }

        inline const PlaneBasis& get_plane() const { return plane_; }
        inline PlaneBasis& get_plane() { return plane_; }
        inline const Gravity& get_gravity() const { return gravity_; }
        inline Gravity& get_gravity() { return gravity_; }

        inline const std::vector<Contact>& get_begin_contacts_solid() const
            { return begin_contacts_solid_; }
        inline const std::vector<Contact>& get_end_contacts_solid() const
            { return end_contacts_solid_; }
        inline const std::vector<Contact>& get_begin_contacts_sensor() const
            { return begin_contacts_sensor_; }
        inline const std::vector<Contact>& get_end_contacts_sensor() const
            { return end_contacts_sensor_; }

        static Scope<PhysicsEngine2D> create(Scene* owner);
    protected:
        inline Scene& get_owner() { return *owner_; }
        inline const Scene& get_owner() const { return *owner_; }
        entt::registry& get_registry();

        inline bool is_plane_dirty() const { return plane_dirty_; }
        inline void plane_applied() { plane_dirty_ = false; }
    protected:
    // synced/refreshed in on_update
        std::vector<Contact> begin_contacts_solid_{};
        std::vector<Contact> end_contacts_solid_{};
        std::vector<Contact> begin_contacts_sensor_{};
        std::vector<Contact> end_contacts_sensor_{};
    private:
        Scene* owner_;
        PlaneBasis plane_{ glm::vec3(0.0f, 0.0f, 1.0f) };
        bool plane_dirty_{ false };
        Gravity gravity_{};
    };
}
