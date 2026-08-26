module;

#include <vector>
#include <unordered_map>
#include <entt/entt.hpp>
namespace rke { class Scene; }

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

        inline const std::vector<Contact>& get_begin_contacts() const
            { return begin_contacts_; }
        inline const std::vector<Contact>& get_end_contacts() const
            { return end_contacts_; }

        void register_shape_entity(uint64 shape_id, uint32 entity_handle);
        void unregister_shape_entity(uint64 shape_id);
        uint32 get_entity_from_shape(uint64 shape_id) const;

        static Scope<PhysicsEngine2D> create(Scene* owner);
    protected:
        inline Scene& get_owner() { return *owner_; }
        entt::registry& get_registry();
    protected:
    // synced/refreshed in on_update
        std::vector<Contact> begin_contacts_{};
        std::vector<Contact> end_contacts_{};
    private:
        Scene* owner_;
        std::unordered_map<uint64, uint32> shape_to_entity_{};
    };
}
