module;

#include <box2d/box2d.h>
#include <box2d/math_functions.h>

module PhysicsEngine2D;
import :box2D;

import Log;
import Types;
import Gravity2D;
import Renderer2D;
import Components;
import Project;
import Application;
import Scene;

namespace {
    using namespace rke;

    b2Filter get_filter(const PhysicsLayers& layers, uint8 layer_index)
    {
        b2Filter filter{};
        filter.categoryBits = layers.get_category_bit(layer_index);
        filter.maskBits		= layers.get_mask(layer_index);
        filter.groupIndex	= 0;
        return filter;
    }

    static b2BodyType to_b2_body_type(BodyType type)
    {
        switch(type)
        {
        case BodyType::Simulated:   return b2_dynamicBody;
        case BodyType::Unsimulated: return b2_kinematicBody;
        }
        return b2_kinematicBody;
    }

// for one-way callback
    static bool is_one_way(Entity entity)
    {
        return entity.valid() && entity.has<BoxCollider2DComponent>()
            && entity.get<BoxCollider2DComponent>().type == ColliderType::OneWay;
    }

    static bool is_one_way_allowed(Entity platform, b2ShapeId other_shape)
    {
        const auto& tc { platform.get<TransformComponent>() };
        const auto& bcc{ platform.get<BoxCollider2DComponent>() };

        b2Vec2 platform_pos{ b2Body_GetPosition(b2Shape_GetBody(std::bit_cast<b2ShapeId>(bcc.shape_id))) };
        b2Vec2 other_pos{ b2Body_GetPosition(b2Shape_GetBody(other_shape)) };

        // allow only when the other body's center is above the platform top
        float platform_half_height{ bcc.half_extent.y *
            std::abs(tc.scale.y) * Renderer2D::quad_size.y };
        return other_pos.y > platform_pos.y + platform_half_height;
    }
}

namespace rke
{
// public
    box2DPhysicsEngine2D::box2DPhysicsEngine2D(Scene* owner)
        : PhysicsEngine2D(owner), physics_world_(b2_nullWorldId)
    {
        get_registry().on_destroy<Rigidbody2DComponent>()
            .connect<&on_physics_com_destroy>();
        get_registry().on_destroy<BoxCollider2DComponent>()
            .connect<&on_physics_collider_destroy>();
    }

    void box2DPhysicsEngine2D::on_runtime_start()
    {
        shape_to_entity_ .clear();
        shape_size_cache_.clear();

    // Create physics world
        b2WorldDef world_def{ b2DefaultWorldDef() };
        world_def.gravity = std::bit_cast<b2Vec2>(get_owner().get_gravity());

        physics_world_ = b2CreateWorld(&world_def);
        b2World_SetPreSolveCallback(physics_world_,
            &box2DPhysicsEngine2D::one_way_pre_solve, this);

    // Instantiate bodies & shapes for existing entities
        sync_all_to_body();
    }

    void box2DPhysicsEngine2D::on_runtime_stop()
    {
        shape_to_entity_ .clear();
        shape_size_cache_.clear();

        if(empty()) return;
        sync_all_from_body();

        auto rbc_view{ get_registry().view<Rigidbody2DComponent>() };
        for(entt::entity ent : rbc_view)
        {
            auto& rbc{ rbc_view.get<Rigidbody2DComponent>(ent) };
            rbc.body_id = std::bit_cast<uint64>(b2_nullBodyId);
        }

        auto bcc_view{ get_registry().view<BoxCollider2DComponent>() };
        for(entt::entity ent : bcc_view)
        {
            auto& bcc{ bcc_view.get<BoxCollider2DComponent>(ent) };
            bcc.shape_id = std::bit_cast<uint64>(b2_nullShapeId);
        }

        b2DestroyWorld(physics_world_);
        physics_world_ = b2_nullWorldId;
    }

    void box2DPhysicsEngine2D::on_update(double dt)
    {
        if(empty()) return;
        
    // sync: towards b2Body
        sync_all_to_body();

        begin_contacts_solid_.clear();
        end_contacts_solid_.clear();
        begin_contacts_sensor_.clear();
        end_contacts_sensor_.clear();

        b2World_Step(physics_world_, dt, 4); // force/mass -> velocity -> translation

        b2ContactEvents contact_events{ b2World_GetContactEvents(physics_world_) };
        for(int i{}; i < contact_events.beginCount; i++)
        {
            begin_contacts_solid_.push_back ({
                get_entity_from_shape(contact_events.beginEvents[i].shapeIdA),
                get_entity_from_shape(contact_events.beginEvents[i].shapeIdB)
            });
        }
        for(int i{}; i < contact_events.endCount; i++)
        {
            end_contacts_solid_.push_back ({
                get_entity_from_shape(contact_events.endEvents[i].shapeIdA),
                get_entity_from_shape(contact_events.endEvents[i].shapeIdB)
            });
        }

        b2SensorEvents sensor_events{ b2World_GetSensorEvents(physics_world_) };
        for(int i{}; i < sensor_events.beginCount; i++)
        {
            begin_contacts_sensor_.push_back ({
                get_entity_from_shape(sensor_events.beginEvents[i].sensorShapeId ),
                get_entity_from_shape(sensor_events.beginEvents[i].visitorShapeId)
            });
        }
        for(int i{}; i < sensor_events.endCount; i++)
        {
            end_contacts_sensor_.push_back ({
                get_entity_from_shape(sensor_events.endEvents[i].sensorShapeId ),
                get_entity_from_shape(sensor_events.endEvents[i].visitorShapeId)
            });
        }

    // sync: from b2Body
        sync_all_from_body();
    }

    bool box2DPhysicsEngine2D::empty() const { return B2_IS_NULL(physics_world_); }

    void box2DPhysicsEngine2D::apply_force(Entity entity, glm::vec2 force)
    {
        if(empty()) return;
        if(!entity.valid() || !entity.has<Rigidbody2DComponent>()) return;

        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        if(rbc.type != BodyType::Simulated) return;
        b2BodyId body{ std::bit_cast<b2BodyId>(rbc.body_id) };
        if(B2_IS_NULL(body)) return;

        b2Body_ApplyForceToCenter(body, { force.x, force.y }, true);
    }

// private
    void box2DPhysicsEngine2D::unregister_shape_entity(b2ShapeId shape_id)
    {
        if(B2_IS_NULL(shape_id)) return;
        shape_to_entity_.erase(shape_id);
        CORE_ASSERT(shape_id.index1 < shape_size_cache_.size(),
            u8"box2DPhysicsEngine2D: Out of bound of size-cache!");
        shape_size_cache_[shape_id.index1] = glm::vec2(0.0f);
    }

    uint32 box2DPhysicsEngine2D::get_entity_from_shape(b2ShapeId shape_id) const
    {
        auto it{ shape_to_entity_.find(shape_id) };
        if(it != shape_to_entity_.end()) return it->second;
        return entity_id_null;
    }

    void box2DPhysicsEngine2D::ensure_body(Entity entity)
    {
        if(!entity.valid() || !entity.has<Rigidbody2DComponent>())
            { CORE_ERROR(u8"box2DPhysicsEngine2D: Entity not valid!"); return; }

        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        if(B2_IS_NON_NULL(std::bit_cast<b2BodyId>(rbc.body_id))) return;

        const auto& tc{ entity.get<TransformComponent>() };
        b2BodyDef body_def{ b2DefaultBodyDef() };
        body_def.type = to_b2_body_type(rbc.type);
        body_def.position = {
            tc.translation.x + Renderer2D::quad_centre.x,
            tc.translation.y + Renderer2D::quad_centre.y
        };
        body_def.rotation = b2MakeRot(glm::radians(tc.rotation.z));
        body_def.fixedRotation = rbc.rotation_fixed;

        b2BodyId new_body_id{ b2CreateBody(physics_world_, &body_def) };
        CORE_ASSERT(B2_IS_NON_NULL(new_body_id), u8"box2dPhysicsEngine2D: Body id null!");
        rbc.body_id = std::bit_cast<uint64>(new_body_id);
    }

    void box2DPhysicsEngine2D::destroy_body(Entity entity)
    {
        if(!entity.valid() || !entity.has<Rigidbody2DComponent>())
            { CORE_ERROR(u8"box2DPhysicsEngine2D: Entity not valid!"); return; }

        if(entity.has<BoxCollider2DComponent>()) destroy_shape(entity);

        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        auto body_id{ std::bit_cast<b2BodyId>(rbc.body_id) };
        if(B2_IS_NULL(body_id)) return;

        b2DestroyBody(body_id);
        rbc.body_id = std::bit_cast<uint64>(b2_nullBodyId);
    }

    void box2DPhysicsEngine2D::create_shape(Entity entity, const PhysicsLayers& layers)
    {
        if(!entity.valid()
        || !entity.has<Rigidbody2DComponent>()
        || !entity.has<BoxCollider2DComponent>()) {
            CORE_ERROR(u8"box2DPhysicsEngine2D: Entity not valid!");
            return;
        }

        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        auto body_id{ std::bit_cast<b2BodyId>(rbc.body_id) };
        if(B2_IS_NULL(body_id)) {
            CORE_ERROR(u8"box2DPhysicsEngine2D: Body id null!");
            return;
        }

        auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };
        if(B2_IS_NON_NULL(std::bit_cast<b2ShapeId>(bcc.shape_id)))
            { CORE_ERROR(u8"box2DPhysicsEngine2D: Already has shape!"); return; }

        const auto& tc{ entity.get<TransformComponent>() };
        float size_x{ std::abs(tc.scale.x) * Renderer2D::quad_size.x };
        float size_y{ std::abs(tc.scale.y) * Renderer2D::quad_size.y };
        if(size_x < 0.001f || size_y < 0.001f) return;

        b2Polygon box_geometry{ b2MakeOffsetBox
        (
            bcc.half_extent.x * size_x,
            bcc.half_extent.y * size_y,
            b2Vec2(bcc.offset.x, bcc.offset.y), // centre
            b2MakeRot(0.0f)
        )};

        b2ShapeDef shape_def{ b2DefaultShapeDef() };
        shape_def.density = bcc.density;
        shape_def.material.friction = bcc.friction;
        shape_def.material.restitution = bcc.restitution;
        shape_def.filter = get_filter(layers, bcc.layer_index);
        switch(bcc.type)
        {
        case ColliderType::Sensor:
            shape_def.isSensor = true;
            shape_def.enableSensorEvents = true;
            break;
        case ColliderType::OneWay:
            shape_def.enablePreSolveEvents = true;
            break;
        default: break;
        }
        shape_def.enableContactEvents = true;
        shape_def.userData = reinterpret_cast<void*>(static_cast<uintptr>(entity.get_handle()));

        b2ShapeId shape_id{ b2CreatePolygonShape(body_id, &shape_def, &box_geometry) };
        CORE_ASSERT(B2_IS_NON_NULL(shape_id), u8"box2dPhysicsEngine: Shape id null!");

        bcc.shape_id = std::bit_cast<uint64>(shape_id);
        rbc.mass = b2Body_GetMass(body_id);

    // register: null id will NEVER be registered
        shape_to_entity_.emplace(shape_id, entity.get_handle());
        if(shape_id.index1 >= shape_size_cache_.size())
            shape_size_cache_.resize(2 * shape_id.index1, glm::vec2(0.0f));
        shape_size_cache_[shape_id.index1] =
            glm::vec2(bcc.half_extent.x * size_x, bcc.half_extent.y * size_y);
    }

    void box2DPhysicsEngine2D::destroy_shape(Entity entity)
    {
        if(!entity.valid() || !entity.has<BoxCollider2DComponent>())
            { CORE_ERROR(u8"box2DPhysicsEngine2D: Entity not valid!"); return; }

        auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };
        b2ShapeId shape_id{ std::bit_cast<b2ShapeId>(bcc.shape_id) };
        if(B2_IS_NULL(shape_id)) return;

        unregister_shape_entity(shape_id);
        b2DestroyShape(shape_id, true);
        bcc.shape_id = std::bit_cast<uint64>(b2_nullShapeId);

        if(entity.has<Rigidbody2DComponent>())
        {
            auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
            rbc.mass = b2Body_GetMass(std::bit_cast<b2BodyId>(rbc.body_id));
        }
    }

    void box2DPhysicsEngine2D::rebuild_shape(Entity entity, const PhysicsLayers& layers)
    {
        destroy_shape(entity);
        create_shape(entity, layers);
    }

    bool box2DPhysicsEngine2D::shape_spec_changed(Entity entity, const PhysicsLayers& layers) const
    {
        CORE_ASSERT(entity.has<BoxCollider2DComponent>(),
            u8"box2DPhysicsEngine2D: Entity not valid!");
        const auto& bcc{ entity.get<BoxCollider2DComponent>() };
        b2ShapeId shape{ std::bit_cast<b2ShapeId>(bcc.shape_id) };
        CORE_ASSERT(B2_IS_NON_NULL(shape), u8"box2DPhysicsEngine2D: Shape id null!");

        if(b2Shape_IsSensor(shape) != (bcc.type == ColliderType::Sensor)) return true;
        if(b2Shape_ArePreSolveEventsEnabled(shape) != (bcc.type == ColliderType::OneWay)) return true;

        b2Filter actual{ b2Shape_GetFilter(shape) };
        b2Filter wanted{ get_filter(layers, bcc.layer_index) };
        if(actual.categoryBits != wanted.categoryBits
        || actual.maskBits     != wanted.maskBits    ) return true;

        if(b2Shape_GetDensity(shape) != bcc.density) return true;
        if(b2Shape_GetFriction(shape) != bcc.friction) return true;
        if(b2Shape_GetRestitution(shape) != bcc.restitution) return true;

        const auto& tc{ entity.get<TransformComponent>() };
        glm::vec2 expected {
            bcc.half_extent.x * std::abs(tc.scale.x) * Renderer2D::quad_size.x,
            bcc.half_extent.y * std::abs(tc.scale.y) * Renderer2D::quad_size.y
        };
        CORE_ASSERT(shape.index1 < shape_size_cache_.size(),
            u8"box2DPhysicsEngine2D: Out of bound of size-cache!");
        if(shape_size_cache_[shape.index1] != expected) return true;
        
        return false;
    }

    void box2DPhysicsEngine2D::sync_all_to_body()
    {
        if(empty()) return;

        auto view{ get_registry().view<Rigidbody2DComponent>() };
        for(entt::entity ent : view)
        {
            Entity entity{ get_owner().get_entity(static_cast<uint32>(ent)) };
            if(!entity.valid()) continue;

            auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
            b2BodyId body{ std::bit_cast<b2BodyId>(rbc.body_id) };
            if(B2_IS_NULL(body)) {
                ensure_body(entity);
                body = std::bit_cast<b2BodyId>(rbc.body_id);
            }

        // Body type -> b2Body
            b2BodyType expected{ to_b2_body_type(rbc.type) };
            if(b2Body_GetType(body) != expected)
            {
                b2Body_SetType(body, expected);
                rbc.mass = b2Body_GetMass(body);
            }

        // Transform -> b2Body
            const auto& tc{ entity.get<TransformComponent>() };
            b2Body_SetTransform(body,
                b2Vec2 (
                    tc.translation.x + Renderer2D::quad_centre.x,
                    tc.translation.y + Renderer2D::quad_centre.y
                ),
                b2MakeRot(glm::radians(tc.rotation.z))
            );

        // Velocity -> b2Body
            b2Body_SetLinearVelocity(body, { rbc.velocity.x, rbc.velocity.y });
            b2Body_SetAngularVelocity(body, rbc.angular_velocity);

        // Shape create or rebuild
            if(entity.has<BoxCollider2DComponent>())
            {
                Project* project{ app().get_project() };
                CORE_ASSERT(project, u8"box2dPhysicsEngine2D: Project null!");
                const auto& physics_layers{ project->get_config().physics_layers };
                if(B2_IS_NULL(std::bit_cast<b2ShapeId>
                    (entity.get<BoxCollider2DComponent>().shape_id)))
                    create_shape(entity, physics_layers);
                else if(shape_spec_changed(entity, physics_layers))
                    rebuild_shape(entity, physics_layers);
            }
        }
    }

    void box2DPhysicsEngine2D::sync_all_from_body()
    {
        auto view{ get_registry().view<Rigidbody2DComponent>() };
        for(entt::entity ent : view)
        {
            Entity entity{ get_owner().get_entity(static_cast<uint32>(ent)) };
            if(!entity.valid()) continue;

            auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
            b2BodyId body{ std::bit_cast<b2BodyId>(rbc.body_id) };
            if(B2_IS_NULL(body)) continue;

        // b2Velocity -> RigidBody
            b2Vec2 velocity{ b2Body_GetLinearVelocity(body) };
            rbc.velocity = { velocity.x, velocity.y };
            rbc.angular_velocity = b2Body_GetAngularVelocity(body);

        // b2Pos & b2Rot -> Transform
            if(rbc.type == BodyType::Simulated)
            {
                b2Vec2 position{ b2Body_GetPosition(body) };
                b2Rot  rotation{ b2Body_GetRotation(body) };

                auto& tc{ entity.get_mut<TransformComponent>() };
                tc.translation.x = position.x - Renderer2D::quad_centre.x;
                tc.translation.y = position.y - Renderer2D::quad_centre.y;
                tc.rotation.z = glm::degrees(b2Rot_GetAngle(rotation));
            }
        }
    }

// callback for box2d
    bool box2DPhysicsEngine2D::allow_one_way_contact(b2ShapeId shape_a, b2ShapeId shape_b)
    {
        Entity ent_a{ get_owner().get_entity(get_entity_from_shape(shape_a)) };
        if(is_one_way(ent_a)) return is_one_way_allowed(ent_a, shape_b);

        Entity ent_b{ get_owner().get_entity(get_entity_from_shape(shape_b)) };
        if(is_one_way(ent_b)) return is_one_way_allowed(ent_b, shape_a);

        return true;
    }

    bool box2DPhysicsEngine2D::one_way_pre_solve
        (b2ShapeId shape_a, b2ShapeId shape_b, b2Manifold* manifold, void* context)
    {
        auto& engine{ *(reinterpret_cast<box2DPhysicsEngine2D*>(context)) };
        return engine.allow_one_way_contact(shape_a, shape_b);
    }

// callback for EnTT
    void box2DPhysicsEngine2D::on_physics_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.physics_engine, u8"box2DPhysicsEngine2D: Null!");
        if(ctx.physics_engine->empty()) return;

        auto& engine{ *static_cast<box2DPhysicsEngine2D*>(ctx.physics_engine) };
        Entity entity{ engine.get_owner().get_entity(static_cast<uint32>(ent)) };
        engine.destroy_body(entity);
    }

    void box2DPhysicsEngine2D::on_physics_collider_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.physics_engine, u8"box2DPhysicsEngine2D: Null!");
        if(ctx.physics_engine->empty()) return;

        auto& engine{ *static_cast<box2DPhysicsEngine2D*>(ctx.physics_engine) };
        Entity entity{ engine.get_owner().get_entity(static_cast<uint32>(ent)) };
        engine.destroy_shape(entity);
    }
}
