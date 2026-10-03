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

    bool Entity::valid() const
    {
        if(empty() || !owner_scene_) return false;
        return owner_scene_->is_handle_valid(handle_);
    }

    bool Entity::operator==(const Entity& other) const
    {
        return handle_ == other.handle_ &&
          owner_scene_ == other.owner_scene_;
    }

    bool Entity::operator!=(const Entity& other) const { return !operator==(other); }

    UUID Entity::get_uuid() const
    {
        if(!valid()) return UUID(0);
        return get<IdentityComponent>().uuid;
    }

    StringView Entity::get_tag() const
    {
        if( empty()) return StringView(u8"Null");
        if(!valid()) return StringView(u8"Invalid");
        return StringView(get<IdentityComponent>().tag);
    }

    const Mesh* Entity::get_mesh() const
    {
        if(!valid()) return nullptr;
        if(has<SpriteComponent>()) return get<SpriteComponent>().quad;
        // if(has<ModelComponent>()) return get<ModelComponent>().mesh; // future
        return nullptr;
    }

    Entity Entity::get_parent() const
        { return owner_scene_->get_parent(*this); }

    std::pair<const EntityHandle*, Size> Entity::get_children() const
        { return owner_scene_->get_children(*this); }

    Entity Entity::create_child()
    {
        if(!valid()) return {}; // don't create
        return owner_scene_->create_entity(*this);
    }

    void Entity::detach_child(Entity child)
        { owner_scene_->detach_child(*this, child); }

    void Entity::detach_all_children()
        { owner_scene_->detach_all_children(*this); }

    WorldTransform Entity::get_world_transform() const
    {
        WorldTransform world{};
        if(!valid()) return world;

        auto& reg{ *owner_scene_->registry_ };
        std::vector<EntityHandle> chain{ owner_scene_->get_parent_chain(*this) };
        for(auto it{ chain.rbegin() }; it != chain.rend(); ++it)
        {
            EntityHandle handle{ *it };
            CORE_ASSERT(owner_scene_->is_handle_valid(handle),
                u8"Entity: Parent handle invalid!");
            world.compose_with(reg.get
                <TransformComponent>(static_cast<entt::entity>(handle)));
        }
        return world;
    }

    glm::vec3 Entity::to_local_delta(glm::vec3 world_delta) const
    {
        const Entity parent{ get_parent() };
        if(!parent.valid()) return world_delta;

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

    void Entity::check_assert() const { CORE_ASSERT(valid(), u8"Entity: Invalid!"); }

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
        Scope<Scene> new_scene{ create_scope<Scene>(owner_, name_) };
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

        // after IdentityComponents are copied
        new_scene->for_each_entity([&new_scene](Entity entity)
            { new_scene->entity_map_[entity.get_uuid()] = entity.handle_; });

        new_scene->set_selected_entity(get_selected_entity().get_uuid());
        new_scene->set_master_camera(get_master_camera().get_uuid());
        new_scene->set_demo_camera(get_demo_camera().get_uuid());

        return new_scene;
    }

    Entity Scene::create_entity(Entity parent, const String& tag, UUID uuid)
    {
        if(!parent.empty() && !vertified(parent)) parent = {};
        Entity entity{ static_cast<EntityHandle>(registry_->create()), this };
        CORE_ASSERT(entity.handle_ != entity_handle_null,
            u8"Scene: Failed to create entity!");

        relations_.emplace(entity.handle_, Relation{});
        set_parent(entity, parent);

        entity.emplace<IdentityComponent>(tag.c_str(), uuid);
        if(!uuid.empty()) entity_map_[entity.get_uuid()] = entity.handle_;
        entity.emplace<TransformComponent>();

        mark_modified();
        return entity;
    }

    Entity Scene::copy_entity(Entity entity)
    {
        if(!vertified(entity)) return {};

    // Copies the whole subtree, not just the entity
        std::unordered_map<EntityHandle, EntityHandle> made{};
        const EntityHandle root_handle{ get_parent(entity).get_handle() };
        made.emplace(root_handle, root_handle);
        std::vector<EntityHandle> todo{ entity.get_handle() };
        for(Size i{}; i < todo.size()/* fresh every turn */; i++)
        {
            const EntityHandle src_handle{ todo[i] };

            Entity src{ get_entity(src_handle) };
            if(!src.valid()) continue;

            const EntityHandle src_parent{ get_parent(src).get_handle() };
            const auto it{ made.find(src_parent) };
            const UUID new_uuid{ temporary_ ? UUID(0) : UUID() };
            Entity copy{ create_entity (
                (it == made.end()) ? Entity{} : get_entity(it->second),
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
            auto [data, count]{ get_children(src) };
            for(Size i{}; i < count; i++) todo.push_back(data[i]);
        }
        const auto it{ made.find(entity.get_handle()) };
        return (it == made.end()) ? Entity{} : get_entity(it->second);
    }

    void Scene::destroy_entity(Entity entity)
    {
        if(!vertified(entity)) return;

        if(entity == selected_entity_) set_selected_entity(Entity{});
        if(entity == master_cam_) set_master_camera(Entity{});
        if(entity == demo_cam_) set_demo_camera(Entity{});

        detach_all_children(entity); // requires entity.get_parent()
        auto parent_it{ relations_.find(get_parent(entity).get_handle()) };
        if(parent_it != relations_.end())
            std::erase(parent_it->second.children, entity.get_handle());
        
        relations_.erase(entity.get_handle()); // may modify
        to_destroy_.push_back(entity.get_handle());
        mark_modified();
    }

    bool Scene::has_entity(UUID uuid) const
    {
        if(uuid.empty()) return false;
        return entity_map_.contains(uuid);
    }

    bool Scene::is_handle_valid(EntityHandle handle) const
        { return registry_->valid(static_cast<entt::entity>(handle)); }

    Entity Scene::get_entity(EntityHandle handle)
    {
        if(handle == entity_handle_null) return {};
        if(is_handle_valid(handle)) return Entity(handle, this);
        CORE_WARN(u8"Scene: Entity handle '{}' not valid!",
            static_cast<uint32>(handle));
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
        if(handle == entity_handle_null) return {};
        if(is_handle_valid(handle)) return Entity(handle, const_cast<Scene*>(this));
        CORE_WARN(u8"Scene: Entity handle not valid");
        return {};
    }

    const Entity Scene::get_entity(UUID uuid) const
    {
        if(uuid.empty()) return {};
        auto it{ entity_map_.find(uuid) };
        if(it == entity_map_.end())
        {
            CORE_WARN(u8"Scene: Entity UUID '{}' not found!", uuid.value());
            return {};
        }
        return Entity(it->second, const_cast<Scene*>(this));
    }

    void Scene::set_selected_entity(Entity entity)
    {
        if(entity.empty()) { selected_entity_ = {}; return; }
        if(!vertified(entity)) return;
        selected_entity_ = entity;
        if(entity.has<CameraComponent>()) set_demo_camera(entity);
    }

    void Scene::set_master_camera(Entity entity)
    {
        if(entity.empty()) { master_cam_ = {}; return; }
        if(entity == master_cam_ || !vertified(entity)) return;
        if(!entity.has<CameraComponent>()) {
            CORE_WARN(u8"Scene: Entity '{}' isn't a camera!", entity.get_tag());
            return;
        }
        master_cam_ = entity;
        mark_modified();
    }

    void Scene::set_demo_camera(Entity entity)
    {
        if(entity.empty()) { demo_cam_ = {}; return; }
        if(entity == demo_cam_ || !vertified(entity)) return;
        if(!entity.has<CameraComponent>()) {
            CORE_WARN(u8"Scene: Entity '{}' isn't a camera!", entity.get_tag());
            return;
        }
        demo_cam_ = entity;
    }

    bool Scene::set_parent(Entity child, Entity parent, Entity before)
    {
        if(!vertified(child)) return false;
        if(!parent.empty() && !vertified(parent)) return false;
        if(child == parent) {
            CORE_WARN(u8"Scene: Entity '{}' "
                u8"can't be its own parent!", child.get_tag());
            return false;
        }

        const EntityHandle child_handle{ child.get_handle() };
        const EntityHandle new_parent{ parent.get_handle() };

        const auto child_it{ relations_.find(child_handle) };
        if(child_it == relations_.end()) return false;

        const EntityHandle old_parent{ child_it->second.parent };
        if(old_parent != new_parent) // validation check
        {
        // a loop would make get_parent_chain() unbounded and the world transform undefined
            for(Entity ancestor{ parent }; ancestor.valid(); ancestor = get_parent(ancestor))
                if(ancestor == child) {
                    CORE_ERROR(u8"Scene: Can't parent entity '{}' "
                        u8"to its own descendant!", child.get_tag());
                    return false;
                }
        }

    // unbind old relations
        if(auto it{ relations_.find(old_parent) }; it != relations_.end())
            std::erase(it->second.children, child_handle);
        
    // bind new relations, at the requested position
        auto& list{ relations_[new_parent].children };
        const auto pos{ before.empty() ? list.end()
            : std::find(list.begin(), list.end(), before.get_handle()) };
        list.insert(pos, child_handle);

        relations_[child_handle].parent = new_parent;

        mark_modified();
        return true;
    }

    void Scene::detach_child(Entity entity, Entity child)
    {
        if(!vertified(entity) || !vertified(child)) return;
        auto it{ relations_.find(entity.get_handle()) };
        if(it == relations_.end()) return;
        
        auto& children{ it->second.children };
        const EntityHandle child_handle{ child.get_handle() };
        if(std::ranges::contains(children, child_handle))
        {
            set_parent(child, get_entity(it->second.parent));
            // apply transform...
            std::erase(children, child_handle);
        }
        else CORE_WARN(u8"Entity: Child not found!");
    }

    void Scene::detach_all_children(Entity entity)
    {
        if(!vertified(entity)) return;
        auto it{ relations_.find(entity.get_handle()) };
        if(it == relations_.end()) return;
        
        auto& orphans{ it->second.children };
        for(EntityHandle child : orphans)
        {
            set_parent(child, it->second.parent);
            // apply transform...
        }
        orphans.clear();
    }

    Entity Scene::get_parent(Entity entity) const
    {
        if(!vertified(entity)) return {};
        auto it{ relations_.find(entity.get_handle()) };
        if(it == relations_.end()) return {};
        return get_entity(it->second.parent);
    }

    std::vector<EntityHandle> Scene::get_parent_chain(Entity entity) const
    {
        std::vector<EntityHandle> chain{};
        if(!entity.belongs_to(this) || !entity.valid()) return chain;

        for(Entity current{ entity }; current.valid();)
        {
            chain.push_back(current.get_handle());
            current = get_parent(current);
            if(current.empty()) break;
            if(std::find(chain.begin(), chain.end(), current.get_handle()) != chain.end())
            {
                CORE_ERROR(u8"Scene: Entity parent chain looped!");
                chain.clear(); break;
            }
        }
        return chain;
    }

    std::pair<const EntityHandle*, Size> Scene::get_children(Entity entity) const
    {
        if(!entity.empty() && !vertified(entity)) return { nullptr, 0 };
        const auto it{ relations_.find(entity.get_handle()) };
        if(it == relations_.end()) return { nullptr, 0 };
        
        const auto& children{ it->second.children };
        return { children.data(), children.size() };
    }

    std::pair<const EntityHandle*, Size> Scene::get_roots() const
        { return get_children(Entity{}); }

    void Scene::order_entity(Entity entity, Entity before)
    {
        if(!vertified(entity)) return;
        if(before == entity) return; // dropped right onto itself
        (void)set_parent(entity, get_parent(entity), before);
    }

    void Scene::set_physics_plane(glm::vec3 axis) { physics_engine_->set_plane(axis); }

    void Scene::grip_move_entity(Entity entity, glm::vec3 delta, double dt)
    {
        if(!vertified(entity)) return;
        entity.get_mut<TransformComponent>().translation += entity.to_local_delta(delta);

    // clear previously-accumulated(force/mass * dt) velocity
        if(entity.has<Rigidbody2DComponent>())
        {
            auto& rbc{ entity.get_mut<Rigidbody2DComponent>() };
            if(dt > 0.0) {
                rbc.velocity = physics_engine_->get_plane()
                    .to_uv(delta) / static_cast<float>(dt);
                rbc.angular_velocity = 0.0f;
            }
        }
    }

    void Scene::apply_force(Entity entity, glm::vec2 force)
    {
        if(!in_runtime() || !vertified(entity)) return;
        physics_engine_->apply_force(entity, force);
    }

    void Scene::apply_acceleration(Entity entity, glm::vec2 acceleration)
    {
        if(!in_runtime() || !vertified(entity)) return;
        if(!entity.has<Rigidbody2DComponent>()) return;
        const auto& rbc{ entity.get<Rigidbody2DComponent>() };
        physics_engine_->apply_force(entity, acceleration * rbc.mass);
    }

    void Scene::animator_play(Entity entity)
    {
        if(!vertified(entity)) return;
        animator_system_->play(entity.get_handle());
    }

    void Scene::animator_stop(Entity entity)
    {
        if(!vertified(entity)) return;
        animator_system_->stop(entity.get_handle());
    }

    void Scene::animator_pause(Entity entity)
    {
        if(!vertified(entity)) return;
        animator_system_->pause(entity.get_handle());
    }

    void Scene::animator_resume(Entity entity)
    {
        if(!vertified(entity)) return;
        animator_system_->resume(entity.get_handle());
    }

    bool Scene::animator_playing(Entity entity)
    {
        if(!vertified(entity)) return false;
        return animator_system_->playing(entity.get_handle());
    }

    bool Scene::animator_paused(Entity entity)
    {
        if(!vertified(entity)) return false;
        return animator_system_->paused(entity.get_handle());
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
            auto& nsc{ registry_->get<NativeScriptComponent>(ent) };
            if(nsc.script_type == script_type_null) continue;
            String name{ old_reg.get_script_name(nsc.script_type) };
            nsc.script_type = new_reg.get_script_type(name);
        }
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
        script_manager_->on_mouse_scrolled
            (e.get_x_offset(), e.get_y_offset());
    }

    bool Scene::vertified(Entity entity) const
    {
        if(entity.belongs_to(this) && is_handle_valid(entity.get_handle())) return true;
        CORE_WARN(u8"Scene: Entity '{}' doesn't belong to this!", entity.get_tag());
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

    const AnimatorSystem::AnimPlayState* Scene::animator_state(Entity entity)
    {
        if(!entity.belongs_to(this)) return nullptr;
        return animator_system_->get_state_from(entity.get_handle());
    }
}
