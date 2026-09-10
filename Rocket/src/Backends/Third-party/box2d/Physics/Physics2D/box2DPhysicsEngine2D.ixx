module;

#include <bit>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <box2d/box2d.h>
#include <box2d/math_functions.h>

export module PhysicsEngine2D:box2D;

import :Base;
import Scene;
import EntityAccess;
import PhysicsLayers;

namespace std
{
    template<>
    struct hash<b2ShapeId> {
        constexpr size_t operator()(b2ShapeId shape_id) const
            { return hash<size_t>()(std::bit_cast<size_t>(shape_id)); }
    };

    inline bool operator==(const b2ShapeId& lhs, const b2ShapeId& rhs)
        { return std::bit_cast<size_t>(lhs) == std::bit_cast<size_t>(rhs); }
}

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

        void apply_force(Entity entity, glm::vec2 force) override;
    private:
        struct PhysicsState
        {
            b2BodyId  body { b2_nullBodyId  };
            b2ShapeId shape{ b2_nullShapeId };
            glm::vec2 shape_size{ 0.0f }; // last resolved half extent(pixels)
        };

        PhysicsState* find_state(EntityHandle handle) noexcept; // nullptr: no state yet
        PhysicsState& state_of  (EntityHandle handle);          // emplaces on demand

        void destroy_shape(PhysicsState& state); // idempotent
        void destroy_body (PhysicsState& state); // idempotent
        void remove_entity(EntityHandle handle); // idempotent: destroy body/shape + drop state
        void remove_shape_of(EntityHandle handle);

        void register_shape_entity(b2ShapeId shape_id, EntityHandle handle);
        void unregister_shape_entity(b2ShapeId shape_id);
        EntityHandle get_entity_from_shape(b2ShapeId shape_id) const;

        void ensure_body(Entity entity, PhysicsState& state);

        void create_shape(Entity entity, PhysicsState& state, const PhysicsLayers& layers);
        void rebuild_shape(Entity entity, PhysicsState& state, const PhysicsLayers& layers);
        bool shape_spec_changed(Entity entity, const PhysicsState& state,
            const PhysicsLayers& layers) const;

        void sync_all_to_body();
        void sync_all_from_body();

    // callback for box2d
        bool allow_one_way_contact(b2ShapeId shape_a, b2ShapeId shape_b);
        static bool one_way_pre_solve(b2ShapeId shape_a, b2ShapeId shape_b,
            b2Manifold* manifold, void* context);
    // callbacks for EnTT
        static void on_physics_com_destroy(entt::registry& reg, entt::entity ent);
        static void on_physics_collider_destroy(entt::registry& reg, entt::entity ent);
    private:
        b2WorldId physics_world_;
        std::unordered_map<b2ShapeId, EntityHandle> shape_to_entity_{};
        std::unordered_map<EntityHandle, PhysicsState> states_{};
    };
}
