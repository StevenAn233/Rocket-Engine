module;
module Scene;

import Log;
import Components;
import Project;
import PhysicsEngine2D;
import ScriptRegistry;
import ScriptManager;
import AssetsManager;
import Texture;
import SceneHierarchyPanel;

namespace {
    using namespace rke;

    static Entity moving_entity(Entity entity)
    {
        if(!entity.is_valid()) return {};
        const std::vector<EntityHandle> chain{ entity.get_parent_chain() };
        if(chain.empty()) return {};

        Entity root{ entity.get_owner()->get_entity(chain.back()) };
        if(!root.is_valid() || !root.has<Rigidbody2DComponent>()) return {};
        return root;
    }

// An entity that stops being carried and becomes a root gets a body of its own next frame;
// so it takes over the motion it was being carried with and carries on
    static void take_over_motion(Entity entity)
    {
        if(!entity.is_valid() || !entity.has<Rigidbody2DComponent>()) return;

        const glm::vec2 velocity{ entity.get_velocity() };
        const float angular_velocity{ entity.get_angular_velocity() };

        Rigidbody2DComponent& rbc{ entity.get_mut<Rigidbody2DComponent>() };
        rbc.velocity = velocity;
        rbc.angular_velocity = angular_velocity;
    }
}

namespace rke
{
    void WorldTransform::compose_with(const TransformComponent& local)
    {
        matrix   *= local.get_transform();
        rotation *= glm::quat(glm::radians(local.rotation));
        scale    *= local.scale;
    }

    Entity::Entity(EntityHandle handle, Scene* scene)
        : handle_(handle), owner_scene_(scene) {}

    bool Entity::is_valid() const
    {
        if(is_null() || !owner_scene_) return false;
        return owner_scene_->is_handle_valid(handle_);
    }

    UUID Entity::get_uuid() const
    {
        if(!is_valid()) return UUID(0);
        return get<IdentityComponent>().uuid;
    }

    StringView Entity::get_tag() const
    {
        if(is_null()) return StringView(u8"Null");
        if(!is_valid()) return StringView(u8"Invalid");
        return StringView(get<IdentityComponent>().tag);
    }

    const Mesh* Entity::get_mesh() const
    {
        if(!is_valid()) return nullptr;
        if(has<SpriteComponent>()) return get<SpriteComponent>().quad;
        // if(has<ModelComponent>()) return get<ModelComponent>().mesh; // future
        return nullptr;
    }

    bool Entity::operator==(const Entity& other) const
    {
        return handle_ == other.handle_ &&
          owner_scene_ == other.owner_scene_;
    }

    bool Entity::operator!=(const Entity& other) const { return !operator==(other); }

    WorldTransform Entity::get_world_transform() const
    {
        WorldTransform world{};
        if(!is_valid()) return world;

        std::vector<EntityHandle> chain{ get_parent_chain() };
        for(auto it{ chain.rbegin() }; it != chain.rend(); ++it)
        {
            Entity entity{ owner_scene_->get_entity(*it) };
            CORE_ASSERT(entity.is_valid(), u8"Entity: Parent handle invalid!");
            world.compose_with(entity.get<TransformComponent>());
        }
        return world;
    }

    void Entity::set_world_transform(const WorldTransform& world)
    {
        if(!is_valid()) return;

        const WorldTransform parent_world{ get_parent().get_world_transform() };
        const glm::vec3 abs_scale{ glm::abs(parent_world.scale) };
        if(abs_scale.x < 1e-6f || abs_scale.y < 1e-6f || abs_scale.z < 1e-6f)
        {
            CORE_WARN(u8"Entity: Parent is scaled to zero, can't set a world transform!");
            return;
        }

        const glm::mat4 local{ glm::inverse(parent_world.matrix) * world.matrix };

        TransformComponent& tc{ get_mut<TransformComponent>() };
        tc.rotation = glm::degrees(glm::eulerAngles
            (glm::inverse(parent_world.rotation) * world.rotation));
        tc.scale = world.scale / parent_world.scale;
    // whatever puts the anchor where the matrix says, so get_transform() reproduces local
        tc.translation = glm::vec3(local * glm::vec4(tc.anchor, 1.0f));
    }

    glm::vec3 Entity::to_local_delta(glm::vec3 world_delta) const
    {
        if(!is_valid()) return world_delta;
        const Entity parent{ get_parent() };
        if(!parent.is_valid()) return world_delta;

        const WorldTransform parent_world{ parent.get_world_transform() };
        const glm::vec3 abs_scale{ glm::abs(parent_world.scale) };
        if(glm::length(abs_scale) < 1e-6f)
        {
            CORE_ERROR(u8"Entity: Parent is scaled to zero, "
                u8"can't convert a world-space delta!");
            return glm::vec3(0.0f);
        }
        return glm::inverse(glm::mat3(parent_world.matrix)) * world_delta;
    }

    glm::vec3 Entity::compute_centre() const
    {
        const Mesh* mesh{ get_mesh() };
        return glm::vec3(get_world_transform().matrix *
            glm::vec4(mesh ? mesh->get_centre() : glm::vec3(0.0f), 1.0f));
    }

    glm::vec2 Entity::compute_flat_size(const PlaneBasis& plane) const
    {
        const Mesh* mesh{ get_mesh() };
        if(!mesh) return glm::vec2(0.0f);

        const WorldTransform world{ get_world_transform() };
        const glm::vec3 raw_size{ mesh->get_size() * glm::abs(world.scale) };

        const float spin{ glm::radians(plane.angle_of(world.rotation)) };
        const glm::quat untilted{ glm::angleAxis(-spin, plane.get_normal()) * world.rotation };
        const glm::mat3 rotation{ glm::mat3_cast(untilted) };

        const glm::vec2 x_axis{ plane.to_uv(rotation * glm::vec3(1.0f, 0.0f, 0.0f)) };
        const glm::vec2 y_axis{ plane.to_uv(rotation * glm::vec3(0.0f, 1.0f, 0.0f)) };
        return glm::vec2 (
            glm::abs(x_axis.x) * raw_size.x + glm::abs(y_axis.x) * raw_size.y,
            glm::abs(x_axis.y) * raw_size.x + glm::abs(y_axis.y) * raw_size.y
        );
    }

    float Entity::compute_flat_rotation(const PlaneBasis& plane) const
        { return plane.angle_of(get_world_transform().rotation); }

    glm::vec2 Entity::get_velocity() const
    {
        const Entity mover{ moving_entity(*this) };
        if(!mover.is_valid()) return glm::vec2(0.0f);

        const PlaneBasis& plane{ owner_scene_->physics_engine_->get_plane() };
        const Rigidbody2DComponent& rbc{ mover.get<Rigidbody2DComponent>() };

    // measured from the root's mesh centre, which is where the body's position is set
        const glm::vec2 r{ plane.to_uv(compute_centre()) - plane.to_uv(mover.compute_centre()) };
    // w x r in the plane; a positive angular velocity turns counter-clockwise in uv
        return rbc.velocity + rbc.angular_velocity * glm::vec2(-r.y, r.x);
    }

    float Entity::get_angular_velocity() const
    {
        const Entity mover{ moving_entity(*this) };
        if(!mover.is_valid()) return 0.0f;
        return mover.get<Rigidbody2DComponent>().angular_velocity;
    }

    void Entity::check_assert() const { CORE_ASSERT(is_valid(), u8"Entity: Invalid!"); }

    void Entity::check_sprite_com() const
        { CORE_ASSERT(has<SpriteComponent>(), u8"Entity: Doesn't has sprite!"); }

    void Entity::check_texture_com() const
        { CORE_ASSERT(!has<TextureComponent>(), u8"Entity: Already has texture!"); }

    void Entity::check_animator_com() const
        { CORE_ASSERT(!has<AnimatorComponent>(), u8"Entity: Already has animation!"); }

    void Entity::remove_all_sprite_related()
    {
        if(has<TextureComponent>()) remove<TextureComponent>();
        if(has<AnimatorComponent>()) remove<AnimatorComponent>();
        if(has<Rigidbody2DComponent>()) remove<Rigidbody2DComponent>();
        if(has<BoxCollider2DComponent>()) remove<BoxCollider2DComponent>();
    }

    Entity Entity::create_child()
    {
        if(!is_valid()) return {};
        return owner_scene_->create_entity(handle_);
    }

    void Entity::detach_child(Entity child)
    {
        if(!is_valid() || !child.is_valid()) return;
        if(!child.belongs_to(owner_scene_))
        {
            CORE_WARN(u8"Entity: Child not in the same scene!");
            return;
        }
        auto it{ owner_scene_->relations_.find(handle_) };
        if(it == owner_scene_->relations_.end()) return;

        if(!std::ranges::contains(it->second.children, child.get_handle()))
        {
            CORE_WARN(u8"Entity: Child '{}' not found!", child.get_tag());
            return;
        }
        const EntityHandle new_parent{ it->second.parent };
        owner_scene_->set_parent(child.get_handle(), new_parent);
    }

    void Entity::detach_all_children()
    {
        if(!is_valid()) return;
        auto it{ owner_scene_->relations_.find(handle_) };
        if(it == owner_scene_->relations_.end()) return;

        const std::vector<EntityHandle> orphans{ it->second.children }; // copy
        const EntityHandle new_parent{ it->second.parent };
        for(EntityHandle handle : orphans)
            owner_scene_->set_parent(handle, new_parent);
    }

    Entity Entity::get_parent() const
    {
        if(!is_valid()) return {};
        return owner_scene_->get_parent(handle_);
    }

    std::vector<EntityHandle> Entity::get_parent_chain() const
    {
        std::vector<EntityHandle> chain{};
        if(!is_valid()) return chain;

        for(Entity current{ *this }; current.is_valid();)
        {
            chain.push_back(current.get_handle());
            current = current.get_parent();
            if(current.is_null()) break;
            if(std::find(chain.begin(), chain.end(), current.get_handle()) != chain.end())
            {
                CORE_ERROR(u8"Entity: Parent chain looped!");
                chain.clear(); break;
            }
        }
        return chain;
    }

    std::pair<const EntityHandle*, Size> Entity::get_children() const
    {
        if(!is_valid()) return { nullptr, 0 };
        return owner_scene_->get_children(handle_);
    }

    void Entity::grip_move_by(glm::vec3 delta, double dt)
    {
        if(!is_valid()) return;
        get_mut<TransformComponent>().translation += to_local_delta(delta);

    // clear previously-accumulated(force/mass * dt) velocity
        if(has<Rigidbody2DComponent>())
        {
            auto& rbc{ get_mut<Rigidbody2DComponent>() };
            if(dt > 0.0) {
                rbc.velocity = owner_scene_->physics_engine_->
                    get_plane().to_uv(delta) / static_cast<float>(dt);
                rbc.angular_velocity = 0.0f;
            }
        }
    }

    void Entity::force_apply_by(glm::vec2 force)
    {
        if(!is_valid() || !has<Rigidbody2DComponent>()) return;
        if(!owner_scene_->in_runtime()) return;
        owner_scene_->physics_engine_->apply_force(handle_, force);
    }

    void Entity::acc_apply_by(glm::vec2 acc)
    {
        if(!is_valid() || !has<Rigidbody2DComponent>()) return;
        if(!owner_scene_->in_runtime()) return;
        const auto& rbc{ get<Rigidbody2DComponent>() };
        owner_scene_->physics_engine_->apply_force(handle_, acc * rbc.mass);
    }

    void Entity::anim_play()
    {
        if(!is_valid() || !has<AnimatorComponent>()) return;
        owner_scene_->animator_system_->play(handle_);
    }

    void Entity::anim_stop()
    {
        if(!is_valid() || !has<AnimatorComponent>()) return;
        owner_scene_->animator_system_->stop(handle_);
    }

    void Entity::anim_pause()
    {
        if(!is_valid() || !has<AnimatorComponent>()) return;
        owner_scene_->animator_system_->pause(handle_);
    }

    void Entity::anim_resume()
    {
        if(!is_valid() || !has<AnimatorComponent>()) return;
        owner_scene_->animator_system_->resume(handle_);
    }

    bool Entity::is_anim_playing()
    {
        if(!is_valid() || !has<AnimatorComponent>()) return false;
        return owner_scene_->animator_system_->playing(handle_);
    }

    bool Entity::is_anim_paused()
    {
        if(!is_valid() || !has<AnimatorComponent>()) return false;
        return owner_scene_->animator_system_->paused(handle_);
    }

    Scene::Scene(Project* owner, String name)
        : owner_(owner), name_(std::move(name))
    {
        registry_ = create_scope<entt::registry>();
        reset_relations();

        script_manager_ = create_scope<ScriptManager>(this);
        physics_engine_ = PhysicsEngine2D::create(this);
        animator_system_ = create_scope<AnimatorSystem>(this);

        registry_->ctx().emplace<RegistryContext>
        (
            script_manager_.get(),
            physics_engine_.get(),
            animator_system_.get()
        );
    }

    Scene::~Scene() { if(in_runtime()) on_runtime_stop(); clear(); }

    void Scene::set_name(String name)
    {
        if(name.empty()) name_ = u8"Untitled";
        else name_ = std::move(name);
        mark_modified();
    }

    Path Scene::get_path() const
        { return owner_->get_scenes_dir() / (name_ + u8".rkscene"); }

    Scope<Scene> Scene::duplicate(bool temp)
    {
        Scope<Scene> new_scene{ new Scene(owner_, name_) };
        new_scene->temporary_ = temp;

        new_scene->viewport_h_ = viewport_h_;
        new_scene->viewport_w_ = viewport_w_;

        glm::vec3 gval{ physics_engine_->get_gravity().val() };
        new_scene->physics_engine_->set_gravity(gval);
        glm::vec3 axis{ physics_engine_->get_plane_axis() };
        new_scene->physics_engine_->set_plane(axis);

        const auto& storage{ registry_->storage<entt::entity>() };
        const auto* entities_data{ storage.data() };
        const Size count{ storage.size() };
        for(Size i{}; i < count; i++)
            (void)new_scene->registry_->create(entities_data[i]);
        
        components::each([&](auto type_id)
        {
            using ComponentType = decltype(type_id)::Type;
            auto view{ registry_->view<ComponentType>() };
            for(auto it{ view.rbegin() }; it != view.rend(); ++it)
            {
                entt::entity src_entt{ *it };
                const auto& src_com{ registry_->get<ComponentType>(src_entt) };
                new_scene->registry_->
                    emplace_or_replace<ComponentType>(src_entt, src_com);
            }
        });
        new_scene->relations_ = relations_; // keys are EntityHandles, reused verbatim above

        // Set UUID map: after IdentityComponents are copied
        new_scene->for_each_entity([&new_scene, this](Entity entity)
            { new_scene->entity_map_[entity.get_uuid()] = entity.get_handle(); });

        new_scene->set_selected_entity(get_selected_entity().get_uuid());
        new_scene->set_master_camera(get_master_camera().get_uuid());
        new_scene->set_demo_camera(get_demo_camera().get_uuid());

        return new_scene;
    }

    Entity Scene::create_entity(EntityHandle parent, const String& tag, UUID uuid)
    {
        if(!is_handle_null(parent) && !vertified(parent))
            parent = entity_handle_null;
        
        EntityHandle handle{ static_cast<EntityHandle>(registry_->create()) };
        CORE_ASSERT(!is_handle_null(handle), u8"Scene: Failed to create entity!");

        relations_.emplace(handle, Relation{});
        set_parent(handle, parent);

        Entity entity{ get_entity(handle) };
        entity.emplace<IdentityComponent>(tag.c_str(), uuid);
        if(!uuid.empty()) entity_map_[entity.get_uuid()] = handle;
        entity.emplace<TransformComponent>();

        mark_modified();
        return entity;
    }

    Entity Scene::copy_entity(EntityHandle handle)
    {
        if(!vertified(handle)) return {};

    // Copies the whole subtree, not just the entity
        std::unordered_map<EntityHandle, EntityHandle> made{};
        const EntityHandle root_handle{ get_entity(handle).get_parent().get_handle() };
        made.emplace(root_handle, root_handle);
        std::vector<EntityHandle> todo{ handle };
        for(Size i{}; i < todo.size()/* fresh every turn */; i++)
        {
            const EntityHandle src_handle{ todo[i] };

            Entity src{ get_entity(src_handle) };
            if(!src.is_valid()) continue;

            const auto it{ made.find(src.get_parent().get_handle()) };
            const UUID new_uuid{ temporary_ ? UUID(0) : UUID() };
            Entity copy{ create_entity
            (
                (it == made.end()) ? entity_handle_null : it->second,
                src.get<IdentityComponent>().tag, new_uuid
            )};
            mark_modified();

            components::each([&](auto type_id)
            {
                using ComponentType = decltype(type_id)::Type;
                if constexpr(!std::is_same_v<ComponentType, IdentityComponent>)
                    if(src.has<ComponentType>()) copy.emplace_or_replace
                        <ComponentType>(src.get<ComponentType>());
            });

            made.emplace(src_handle, copy.get_handle());

        // then this entity's own children
            auto [data, count]{ src.get_children() };
            for(Size i{}; i < count; i++) todo.push_back(data[i]);
        }
        const auto it{ made.find(handle) };
        return (it == made.end()) ? Entity{} : get_entity(it->second);
    }

    void Scene::destroy_entity(EntityHandle handle)
    {
        Entity entity{ get_entity(handle) };
        if(!entity.is_valid()) return;

        if(handle == selected_entity_) set_selected_entity(entity_handle_null);
        if(handle == master_cam_) set_master_camera(entity_handle_null);
        if(handle == demo_cam_) set_demo_camera(entity_handle_null);

        entity.detach_all_children(); // requires entity.get_parent()
        auto parent_it{ relations_.find(entity.get_parent().get_handle()) };
        if(parent_it != relations_.end())
        {
            auto& children{ parent_it->second.children };
            std::erase(children, handle);
        }
        
        relations_.erase(handle); // may modify
        to_destroy_.push_back(handle);
        mark_modified();
    }

    void Scene::destroy_entity(UUID uuid)
    {
        if(uuid.empty()) return;
        destroy_entity(get_entity(uuid).get_handle());
    }

    bool Scene::has_entity(UUID uuid) const
        { return uuid.empty() ? false : entity_map_.contains(uuid); }

    bool Scene::is_handle_valid(EntityHandle handle) const
        { return registry_->valid(static_cast<entt::entity>(handle)); }

    Entity Scene::get_entity(EntityHandle handle)
    {
        if(is_handle_null (handle)) return {};
        if(is_handle_valid(handle)) return Entity(handle, this);
        CORE_WARN(u8"Scene: Entity handle may be outdated!");
        return {};
    }

    Entity Scene::get_entity(UUID uuid)
    {
        if(uuid.empty()) return {};
        auto it{ entity_map_.find(uuid) };
        if(it == entity_map_.end()) {
            CORE_WARN(u8"Scene: Entity UUID '{}' not found!", uuid.value());
            return {};
        }
        return Entity(it->second, this);
    }

    const Entity Scene::get_entity(EntityHandle handle) const
    {
        if(is_handle_null (handle)) return {};
        if(is_handle_valid(handle)) return Entity(handle, const_cast<Scene*>(this));
        CORE_WARN(u8"Scene: Entity handle may be outdated!");
        return {};
    }

    const Entity Scene::get_entity(UUID uuid) const
    {
        if(uuid.empty()) return {};
        auto it{ entity_map_.find(uuid) };
        if(it == entity_map_.end()) {
            CORE_WARN(u8"Scene: Entity UUID '{}' not found!", uuid.value());
            return {};
        }
        return Entity(it->second, const_cast<Scene*>(this));
    }

    void Scene::set_selected_entity(EntityHandle handle)
    {
        if(!is_handle_null(handle) && !vertified(handle)) return;
        selected_entity_ = handle;
        if(!is_handle_null(handle) &&
            get_selected_entity().has<CameraComponent>())
            set_demo_camera(handle);
    }

    void Scene::set_selected_entity(UUID uuid)
    {
        if(uuid.empty()) return;
        set_selected_entity(get_entity(uuid).get_handle());
    }

    void Scene::set_master_camera(EntityHandle handle)
    {
        if(!is_handle_null(handle) && !vertified(handle)) return;
        if(master_cam_ == handle) return; master_cam_ = handle;
        if(!is_handle_null(handle) && !get_master_camera().has<CameraComponent>())
        {
            CORE_WARN(u8"Scene: Entity '{}' isn't a camera!",
                get_entity(handle).get_tag());
            master_cam_ = entity_handle_null;
        }
        mark_modified();
    }

    void Scene::set_master_camera(UUID uuid)
    {
        if(uuid.empty()) return;
        set_master_camera(get_entity(uuid).get_handle());
    }

    void Scene::set_demo_camera(EntityHandle handle)
    {
        if(!is_handle_null(handle) && !vertified(handle)) return;
        if(demo_cam_ == handle) return; demo_cam_ = handle;
        if(!is_handle_null(handle) && !get_demo_camera().has<CameraComponent>())
        {
            CORE_WARN(u8"Scene: Entity '{}' isn't a camera!",
                get_entity(handle).get_tag());
            demo_cam_ = entity_handle_null;
        }
    }

    void Scene::set_demo_camera(UUID uuid)
    {
        if(uuid.empty()) return;
        set_demo_camera(get_entity(uuid).get_handle());
    }

    bool Scene::set_parent(EntityHandle child, EntityHandle parent, EntityHandle before)
    {
        Entity child_ent{ get_entity(child) };
        if(!child_ent.is_valid()) return false;
        if(!is_handle_null(parent) && !vertified(parent)) return false;
        if(child == parent) {
            CORE_WARN(u8"Scene: Entity '{}' "
                u8"can't be its own parent!", child_ent.get_tag());
            return false;
        }

        const auto child_it{ relations_.find(child) };
        if(child_it == relations_.end()) return false;

        const EntityHandle old_parent{ child_it->second.parent };
        if(old_parent != parent) // validation check
        {
        // a loop would make get_parent_chain() unbounded and the world transform undefined
            for(Entity ancestor{ get_entity(parent) };
                ancestor.is_valid(); ancestor = ancestor.get_parent())
            {
                if(ancestor.get_handle() != child) continue;
                CORE_ERROR(u8"Scene: Can't parent entity '{}' "
                    u8"to its own descendant!", child_ent.get_tag());
                return false;
            }
        }

    // The motion handoff reads the geometry as it stands, so it comes before the splice.
        if(is_handle_null(parent) && !is_handle_null(old_parent))
            take_over_motion(child_ent);

        const bool moves_out {
            (old_parent != parent && !is_handle_null(old_parent)) &&
            (is_handle_null(parent) || get_parent(old_parent).get_handle() == parent)
        };
        const WorldTransform world_before{ child_ent.get_world_transform() };

    // unbind old relations
        if(auto it{ relations_.find(old_parent) }; it != relations_.end())
            std::erase(it->second.children, child);
        
    // bind new relations, at the requested position
        auto& list{ relations_[parent].children };
        const auto pos{ is_handle_null(before) ?
            list.end() : std::find(list.begin(), list.end(), before) };
        list.insert(pos, child);

        relations_[child].parent = parent;

    // basically resume the entity world transform to what it was before
        if(moves_out) child_ent.set_world_transform(world_before);

        mark_modified();
        return true;
    }

    void Scene::order_entity(EntityHandle handle, EntityHandle before)
    {
        if(!vertified(handle)) return;
        if(before == handle) return; // dropped right onto itself
        set_parent(handle, get_parent(handle).get_handle(), before);
    }

    Entity Scene::get_parent(EntityHandle handle) const
    {
        if(!is_handle_null(handle) && !vertified(handle)) return {};
        auto it{ relations_.find(handle) };
        if(it == relations_.end()) return {};
        return get_entity(it->second.parent);
    }

    std::pair<const EntityHandle*, Size> Scene::get_children(EntityHandle handle) const
    {
        if(!is_handle_null(handle) && !vertified(handle)) return {};
        const auto it{ relations_.find(handle) };
        if(it == relations_.end()) return { nullptr, 0 };
        const auto& children{ it->second.children };
        return { children.data(), children.size() };
    }

    void Scene::clear()
    {
        if(in_runtime()) {
            CORE_ERROR(u8"Scene: Can't be cleared during runtime!");
            return;
        }
        registry_ ->clear();
        reset_relations();
        to_destroy_.clear();
        entity_map_.clear();
        demo_cam_ = {};
        master_cam_ = {};
        selected_entity_ = {};
    }

    void Scene::on_update(double dt)
    {
        if(in_runtime())
        {
            script_manager_->on_update(dt);
            physics_engine_->on_update(dt);
            
            script_manager_->dispatch_contacts
            (
                physics_engine_->get_begin_contacts_solid(),
                physics_engine_->get_end_contacts_solid(),
                physics_engine_->get_begin_contacts_sensor(),
                physics_engine_->get_end_contacts_sensor()
            );
        }
        animator_system_->on_update(dt);
        flush_destroy_queue();
    }

    void Scene::on_runtime_start()
    {
        CORE_ASSERT(!in_runtime(), u8"Scene: Already in runtime!");
        in_runtime_ = true;
        script_manager_->on_runtime_start();
        physics_engine_->on_runtime_start();
    }

    void Scene::on_runtime_stop()
    {
        CORE_ASSERT(in_runtime(), u8"Scene: Not in runtime!");
        in_runtime_ = false;
        script_manager_->on_runtime_stop();
        physics_engine_->on_runtime_stop();
    }

    void Scene::set_physics_plane(glm::vec3 axis)
        { physics_engine_->set_plane(axis); }

    void Scene::set_viewport(uint32 width, uint32 height)
    {
        viewport_w_ = width;
        viewport_h_ = height;
        auto view{ registry_->view<CameraComponent>() };
        for(auto cam_entt : view)
        {
            auto& camera_com{ view.get<CameraComponent>(cam_entt) };
            if(!camera_com.aspect_ratio_fixed)
                camera_com.camera.set_viewport(viewport_w_, viewport_h_);
        }
    }

    void Scene::on_mouse_scrolled_runtime(MouseScrolledEvent& e)
    {
        if(!in_runtime()) return;
        script_manager_->on_mouse_scrolled(e.get_x_offset(), e.get_y_offset());
    }

    void Scene::on_script_dylib_hot_reloading(ScriptRegistry& old_reg, ScriptRegistry& new_reg)
    {
        if(in_runtime()) {
            CORE_ERROR(u8"Scene: Can't reload during runtime!");
            return;
        }
        auto view{ registry_->view<NativeScriptComponent>() };
        for(entt::entity ent : view)
        {
            auto& nsc{ view.get<NativeScriptComponent>(ent) };
            if(nsc.script_type == script_type_null) continue;
            String name{ old_reg.get_script_name(nsc.script_type) };
            nsc.script_type = new_reg.get_script_type(name);
        }
    }

    bool Scene::vertified(EntityHandle handle) const
    {
        if(is_handle_valid(handle)) return true;
        CORE_WARN(u8"Scene: Entity handle not valid!");
        return false;
    }

    void Scene::reset_relations()
    {
        relations_.clear();
        relations_.emplace(entity_handle_null, Relation{});
    }

    void Scene::flush_destroy_queue()
    {
        while(!to_destroy_.empty())
        {
            auto handle{ static_cast<entt::entity>(to_destroy_.back()) };
            to_destroy_.pop_back();
            if(!registry_->valid(handle)) continue;

            UUID uuid{ registry_->get<IdentityComponent>(handle).uuid };
            if(!uuid.empty()) entity_map_.erase(uuid);

            registry_->destroy(handle);
        }
    }

    const AnimatorSystem::AnimPlayState* Scene::animator_state(EntityHandle handle)
    {
        if(!vertified(handle)) return nullptr;
        return animator_system_->get_state_from(handle);
    }
}
