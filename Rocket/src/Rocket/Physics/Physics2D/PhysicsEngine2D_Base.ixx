module;

#include <vector>
#include <unordered_map>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
namespace rke { class Scene; class Entity; }

export module PhysicsEngine2D:Base;

import HeapManager;
import Types;

export namespace rke
{
    struct Contact
    {
        uint32 entity_a{ 0xFFFFFFFFu }; // entity_id_null
        uint32 entity_b{ 0xFFFFFFFFu };
    };

    class PhysicsEngine2D
    {
    public:
        PhysicsEngine2D(Scene* scene);
        virtual ~PhysicsEngine2D() = default;

        virtual void on_runtime_start() = 0;
        virtual void on_runtime_stop () = 0;

        virtual void on_update(double dt) = 0;
        virtual bool empty() const = 0;
        virtual void apply_force(Entity entity, glm::vec2 force) = 0;

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
        entt::registry& get_registry();
    protected:
    // synced/refreshed in on_update
        std::vector<Contact> begin_contacts_solid_{};
        std::vector<Contact> end_contacts_solid_{};
        std::vector<Contact> begin_contacts_sensor_{};
        std::vector<Contact> end_contacts_sensor_{};
    private:
        Scene* owner_;
    };
}
