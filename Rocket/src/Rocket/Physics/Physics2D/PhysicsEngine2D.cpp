module;

#include <box2d/box2d.h>
#include <box2d/math_functions.h>

module PhysicsEngine2D;

import Types;
import Log;
import Gravity2D;
import Components;
import Project;
import PhysicsLayers;

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

    static b2BodyType to_b2_body_type(Rigidbody2DComponent::BodyType type)
    {
        switch(type)
        {
        case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
        case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
        case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
        }
        return b2_staticBody;
    }

    static Scene* s_context{};
    static b2WorldId s_physics_world{ b2_nullWorldId };

    static void create_body(Entity entity, const PhysicsLayers& layers)
    {
        auto& tc { entity.get<TransformComponent>() }; // must have
        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };

        b2BodyDef body_def{ b2DefaultBodyDef() };
        body_def.type     = to_b2_body_type(rbc.type);
        body_def.position = { tc.position.x, tc.position.y };
        body_def.rotation = b2MakeRot(glm::radians(tc.rotation.z));
        body_def.fixedRotation = rbc.rotation_fixed;
        // body_def.xx = ...

        rbc.body_id = std::bit_cast<uint64>(b2CreateBody(s_physics_world, &body_def));

        float size_x{ std::abs(tc.size.x) };
        float size_y{ std::abs(tc.size.y) };
        if(entity.has<BoxCollider2DComponent>() && (size_x >= 0.001f) && (size_y >= 0.001f))
        {
            auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };

            b2Polygon box_geometry { b2MakeOffsetBox
            (
                bcc.size.x * size_x,
                bcc.size.y * size_y,
                { bcc.offset.x, bcc.offset.y }, // centre
                b2MakeRot(0.0f)
            )};

            b2ShapeDef shape_def{ b2DefaultShapeDef() };
            shape_def.density = bcc.density;
            shape_def.material.friction    = bcc.friction;
            shape_def.material.restitution = bcc.restitution;
            shape_def.filter = get_filter(layers, bcc.layer_index);
            // shape_def.xx = ...

            bcc.shape_id = std::bit_cast<uint64>(b2CreatePolygonShape
                (std::bit_cast<b2BodyId>(rbc.body_id), &shape_def, &box_geometry));
            // shape is related to body
        }
    }

    static void destroy_body(Entity entity)
    {
        auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        if(B2_IS_NON_NULL(std::bit_cast<b2BodyId>(rbc.body_id)))
        {
            b2DestroyBody(std::bit_cast<b2BodyId>(rbc.body_id));
            rbc.body_id = std::bit_cast<uint64>(b2_nullBodyId);
            if(entity.has<BoxCollider2DComponent>())
            {
                auto& bcc{ entity.get_mut<BoxCollider2DComponent>() };
                bcc.shape_id = std::bit_cast<uint64>(b2_nullShapeId);
            }
        }
    }

    static void on_physics_component_destroyed(entt::registry&, entt::entity e)
    {
        if(!s_context || !s_context->in_runtime()) return;
        destroy_body(s_context->get_entity(e));
    }
}

namespace rke
{
    void PhysicsEngine2D::on_runtime_start(Scene* scene)
    {
        s_context = scene;

        auto& registry{ *scene->registry_ };
        registry.on_destroy<Rigidbody2DComponent>()
            .connect<&on_physics_component_destroyed>();

    // Create physics world
        b2WorldDef world_def{ b2DefaultWorldDef() };
        world_def.gravity = { scene->get_gravity().x, scene->get_gravity().y };
        s_physics_world = b2CreateWorld(&world_def);

    // Instantiate bodies
        auto rbc_view{ registry.view<Rigidbody2DComponent>() };
        for(auto entt : rbc_view)
        {
            Entity entity{ s_context->get_entity(entt) };
            const auto& physics_layers
                { Project::get_active_project()->get_config().physics_layers };
            create_body(entity, physics_layers);
        }
    }

    void PhysicsEngine2D::on_runtime_stop()
    {
        if(B2_IS_NON_NULL(s_physics_world))
        {
            b2DestroyWorld(s_physics_world);
            s_physics_world = b2_nullWorldId;
        }
        s_context = nullptr;
    }

    void PhysicsEngine2D::on_update(float dt)
    {
        if(!s_context || !s_context->in_runtime_) return;
        if(B2_IS_NON_NULL(s_physics_world))
        {
            b2World_Step(s_physics_world, dt, 4);

            // Sync Box2D bodies back to TransformComponent
            auto view{ s_context->registry_->view<Rigidbody2DComponent>() };
            for(auto entt : view)
            {
                Entity entity{ s_context->get_entity(entt) };

                const auto& rbc{ entity.get<Rigidbody2DComponent>() };
                if(B2_IS_NULL(std::bit_cast<b2ShapeId>(rbc.body_id))) continue;

                if(rbc.type != Rigidbody2DComponent::BodyType::Static)
                {
                    b2Vec2 position{ b2Body_GetPosition(std::bit_cast<b2BodyId>(rbc.body_id)) };
                    b2Rot  rotation{ b2Body_GetRotation(std::bit_cast<b2BodyId>(rbc.body_id)) };

                    // update rigid body position & rotation in TransformComponent
                    auto& tc{ entity.get_mut<TransformComponent>() };
                    tc.position.x = position.x;
                    tc.position.y = position.y;

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
    }
}
