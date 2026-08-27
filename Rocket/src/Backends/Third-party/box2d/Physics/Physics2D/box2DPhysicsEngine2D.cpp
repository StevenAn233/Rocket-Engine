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
        case BodyType::Static:    return b2_staticBody;
        case BodyType::Dynamic:   return b2_dynamicBody;
        case BodyType::Kinematic: return b2_kinematicBody;
        }
        return b2_staticBody;
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

// for EnTT
    static void on_physics_com_destroy(entt::registry& reg, entt::entity ent)
    {
        auto& ctx{ reg.ctx().get<Scene::RegistryContext>() };
        CORE_ASSERT(ctx.physics_engine, u8"box2DPhysicsEngine2D: Null!");
        if(ctx.physics_engine->empty()) return;

        auto& rbc{ reg.get<Rigidbody2DComponent>(ent) };
        if(B2_IS_NULL(std::bit_cast<b2BodyId>(rbc.body_id))) return;
        
        b2DestroyBody(std::bit_cast<b2BodyId>(rbc.body_id));
        rbc.body_id = std::bit_cast<uint64>(b2_nullBodyId);
        if(reg.all_of<BoxCollider2DComponent>(ent))
        {
            auto& bcc{ reg.get<BoxCollider2DComponent>(ent) };
            if(B2_IS_NON_NULL(std::bit_cast<b2ShapeId>(bcc.shape_id)))
                ctx.physics_engine->unregister_shape_entity(bcc.shape_id);
            bcc.shape_id = std::bit_cast<uint64>(b2_nullShapeId);
        }
    }
}

namespace rke
{
    box2DPhysicsEngine2D::box2DPhysicsEngine2D(Scene* owner)
        : PhysicsEngine2D(owner), physics_world_(b2_nullWorldId)
    {
        get_registry().on_destroy<Rigidbody2DComponent>()
            .connect<&on_physics_com_destroy>();
    }

    void box2DPhysicsEngine2D::on_runtime_start()
    {
    // Create physics world
        b2WorldDef world_def{ b2DefaultWorldDef() };
        world_def.gravity = std::bit_cast<b2Vec2>(get_owner().get_gravity());
        physics_world_ = b2CreateWorld(&world_def);
        b2World_SetPreSolveCallback(physics_world_,
            &box2DPhysicsEngine2D::one_way_pre_solve, this);

    // Instantiate bodies
        auto rbc_view{ get_registry().view<Rigidbody2DComponent>() };
        for(auto entt : rbc_view)
        {
            Entity entity{ get_owner().get_entity(static_cast<uint32>(entt)) };
            const auto& physics_layers{ app().get_project()->get_config().physics_layers };
            create_body(entity, physics_layers);
        }
    }

    void box2DPhysicsEngine2D::on_runtime_stop()
    {
        if(B2_IS_NON_NULL(physics_world_))
        {
            b2DestroyWorld(physics_world_);
            physics_world_ = b2_nullWorldId;
        }
    }

    void box2DPhysicsEngine2D::on_update(double dt)
    {
        if(!get_owner().in_runtime() || B2_IS_NULL(physics_world_)) return;

        b2World_Step(physics_world_, dt, 4);

    // Sync contact events
        begin_contacts_.clear();
        end_contacts_.clear();

        b2ContactEvents contacts{ b2World_GetContactEvents(physics_world_) };
        for(int i{}; i < contacts.beginCount; i++)
        {
            begin_contacts_.push_back ({
                get_entity_from_shape(std::bit_cast<uint64>(contacts.beginEvents[i].shapeIdA)),
                get_entity_from_shape(std::bit_cast<uint64>(contacts.beginEvents[i].shapeIdB))
            });
        }
        for(int i{}; i < contacts.endCount; i++)
        {
            end_contacts_.push_back ({
                get_entity_from_shape(std::bit_cast<uint64>(contacts.endEvents[i].shapeIdA)),
                get_entity_from_shape(std::bit_cast<uint64>(contacts.endEvents[i].shapeIdB))
            });
        }

        // Sync Box2D bodies back to TransformComponent
        auto view{ get_registry().view<Rigidbody2DComponent>() };
        for(entt::entity ent : view)
        {
            Entity entity{ get_owner().get_entity(static_cast<uint32>(ent)) };

            const auto& rbc{ entity.get<Rigidbody2DComponent>() };
            if(B2_IS_NULL(std::bit_cast<b2ShapeId>(rbc.body_id))) continue;

            if(rbc.type != BodyType::Static)
            {
                b2Vec2 position{ b2Body_GetPosition(std::bit_cast<b2BodyId>(rbc.body_id)) };
                b2Rot  rotation{ b2Body_GetRotation(std::bit_cast<b2BodyId>(rbc.body_id)) };

                // update rigid body position & rotation in TransformComponent
                auto& tc{ entity.get_mut<TransformComponent>() };
                tc.translation.x = position.x - Renderer2D::quad_centre.x;
                tc.translation.y = position.y - Renderer2D::quad_centre.y;

                float radians{ b2Rot_GetAngle(rotation) };
                tc.rotation.z = glm::degrees(radians);
            }
            else
            {
                // Need to update BoxCollider
                // When entity moved by Gizmo or Panel
            }
        }
    }

    bool box2DPhysicsEngine2D::empty() const { return B2_IS_NULL(physics_world_); }

    void box2DPhysicsEngine2D::create_body(Entity entity, const PhysicsLayers& layers)
    {
        if(!entity.valid() || !entity.has<Rigidbody2DComponent>())
            { CORE_ERROR(u8"box2DPhysicsEngine2D: Entity not valid!"); return; }

        auto& tc { entity.get<TransformComponent>() }; // must have
        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };

        b2BodyDef body_def{ b2DefaultBodyDef() };
        body_def.type = to_b2_body_type(rbc.type);
        body_def.position = {
            tc.translation.x + Renderer2D::quad_centre.x,
            tc.translation.y + Renderer2D::quad_centre.y
        };
        body_def.rotation = b2MakeRot(glm::radians(tc.rotation.z));
        body_def.fixedRotation = rbc.rotation_fixed;
        // body_def.xx = ...

        rbc.body_id = std::bit_cast<uint64>(b2CreateBody(physics_world_, &body_def));

        float size_x{ std::abs(tc.scale.x) * Renderer2D::quad_size.x };
        float size_y{ std::abs(tc.scale.y) * Renderer2D::quad_size.y };
        if(entity.has<BoxCollider2DComponent>() && (size_x >= 0.001f) && (size_y >= 0.001f))
        {
            auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };

            b2Polygon box_geometry {
                b2MakeOffsetBox (
                    bcc.half_extent.x * size_x,
                    bcc.half_extent.y * size_y,
                    { bcc.offset.x, bcc.offset.y }, // centre
                    b2MakeRot(0.0f)
                )
            };

            b2ShapeDef shape_def{ b2DefaultShapeDef() };
            shape_def.density = bcc.density;
            shape_def.material.friction    = bcc.friction;
            shape_def.material.restitution = bcc.restitution;
            shape_def.filter = get_filter(layers, bcc.layer_index);
            switch(bcc.type)
            {
            case ColliderType::Sensor: shape_def.isSensor = true; break;
            case ColliderType::OneWay: shape_def.enablePreSolveEvents = true; break;
            default: break;
            }
            shape_def.enableContactEvents = true;
            shape_def.userData = reinterpret_cast<void*>(static_cast<uintptr>(entity.get_handle()));
            // shape_def.xx = ...

            bcc.shape_id = std::bit_cast<uint64>(b2CreatePolygonShape
                (std::bit_cast<b2BodyId>(rbc.body_id), &shape_def, &box_geometry));
            register_shape_entity(bcc.shape_id, entity.get_handle());
            // shape is related to body
        }
    }

    void box2DPhysicsEngine2D::destroy_body(Entity entity)
    {
        if(!entity.valid() || !entity.has<Rigidbody2DComponent>())
            { CORE_ERROR(u8"box2DPhysicsEngine2D: Entity not valid!"); return; }
        
        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        if(B2_IS_NON_NULL(std::bit_cast<b2BodyId>(rbc.body_id)))
        {
            if(entity.has<BoxCollider2DComponent>())
            {
                auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };
                if(B2_IS_NON_NULL(std::bit_cast<b2ShapeId>(bcc.shape_id)))
                    unregister_shape_entity(bcc.shape_id);
            }
            b2DestroyBody(std::bit_cast<b2BodyId>(rbc.body_id));
            rbc.body_id = std::bit_cast<uint64>(b2_nullBodyId);
            if(entity.has<BoxCollider2DComponent>())
            {
                auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };
                bcc.shape_id = std::bit_cast<uint64>(b2_nullShapeId);
            }
        }
    }

    bool box2DPhysicsEngine2D::allow_one_way_contact(b2ShapeId shape_a, b2ShapeId shape_b)
    {
        Entity ent_a{ get_owner().get_entity
            (get_entity_from_shape(std::bit_cast<uint64>(shape_a))) };
        if(is_one_way(ent_a)) return is_one_way_allowed(ent_a, shape_b);

        Entity ent_b{ get_owner().get_entity
            (get_entity_from_shape(std::bit_cast<uint64>(shape_b))) };
        if(is_one_way(ent_b)) return is_one_way_allowed(ent_b, shape_a);

        return true;
    }

    bool box2DPhysicsEngine2D::one_way_pre_solve
        (b2ShapeId shape_a, b2ShapeId shape_b, b2Manifold* manifold, void* context)
    {
        auto& engine{ *(reinterpret_cast<box2DPhysicsEngine2D*>(context)) };
        return engine.allow_one_way_contact(shape_a, shape_b);
    }
}
