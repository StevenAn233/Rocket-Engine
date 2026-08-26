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
    // callback for box2d
        bool allow_one_way_contact(b2ShapeId shape_a, b2ShapeId shape_b);
        static bool one_way_pre_solve(b2ShapeId shape_a, b2ShapeId shape_b,
            b2Manifold* manifold, void* context);
    private:
        b2WorldId physics_world_;
    };
}
