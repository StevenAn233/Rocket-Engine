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
}

import :box2D;

namespace rke
{
    Scope<PhysicsEngine2D> PhysicsEngine2D::create(Scene* owner)
        { return create_scope<box2DPhysicsEngine2D>(owner); }
}
