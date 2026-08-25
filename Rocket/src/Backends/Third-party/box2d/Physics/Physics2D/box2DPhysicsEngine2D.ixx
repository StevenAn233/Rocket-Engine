module;

#include <box2d/box2d.h>
#include <box2d/math_functions.h>

export module PhysicsEngine2D:box2D;

import :Base;
import Scene;
import PhysicsLayers;

namespace rke
{
    class box2DPhysicsEngine2D : public PhysicsEngine2D
    {
    public:
        box2DPhysicsEngine2D(Scene* owner);

        void on_runtime_start() override;
        void on_runtime_stop () override;

        void on_update(double dt) override;

        bool empty() const override;
    private:
        void create_body(Entity entity, const PhysicsLayers& layers);
        void destroy_body(Entity entity);
    private:
        b2WorldId physics_world_;
    };
}
