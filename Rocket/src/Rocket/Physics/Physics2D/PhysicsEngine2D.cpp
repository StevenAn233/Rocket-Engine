module;
module PhysicsEngine2D;

import :Base;
import Log;
import Scene;

namespace rke
{
    PhysicsEngine2D::PhysicsEngine2D(Scene* scene) : owner_(scene)
        { CORE_ASSERT(owner_, u8"PhysicsEngine2D: Owner scene null!"); }
    
    entt::registry& PhysicsEngine2D::get_registry() { return *(owner_->registry_); }

    void PhysicsEngine2D::register_shape_entity(uint64 shape_id, uint32 entity_handle)
        { shape_to_entity_[shape_id] = entity_handle; }

    void PhysicsEngine2D::unregister_shape_entity(uint64 shape_id)
        { shape_to_entity_.erase(shape_id); }

    uint32 PhysicsEngine2D::get_entity_from_shape(uint64 shape_id) const
    {
        auto it{ shape_to_entity_.find(shape_id) };
        if(it != shape_to_entity_.end()) return it->second;
        CORE_ERROR(u8"PhysicsEngine2D: Entity not found!");
        return entity_id_null;
    }
}

import :box2D;

namespace rke
{
    Scope<PhysicsEngine2D> PhysicsEngine2D::create(Scene* owner)
        { return create_scope<box2DPhysicsEngine2D>(owner); }
}
