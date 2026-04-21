module;
module SceneRenderer;

import Log;
import Renderer2D;
import RenderCommand;
import Components;

namespace {
    static bool should_cull(glm::vec3 pos, glm::vec2 size,
        const std::array<glm::vec4, 6>& frustum_planes)
    {
        float radius{ glm::length(size) * 0.5f };
        for(const auto& plane : frustum_planes)
        {
            if((glm::dot(glm::vec3(plane), pos) + plane.w) < -radius)
                return true;
        }
        return false;
    }

    static std::array<glm::vec4, 6> get_planes_normal(const glm::mat4& vp)
    {
        glm::vec4 left  { (vp[0][3] + vp[0][0]), (vp[1][3] + vp[1][0]),
                          (vp[2][3] + vp[2][0]), (vp[3][3] + vp[3][0]) };
        glm::vec4 right { (vp[0][3] - vp[0][0]), (vp[1][3] - vp[1][0]),
                          (vp[2][3] - vp[2][0]), (vp[3][3] - vp[3][0]) };
        glm::vec4 bottom{ (vp[0][3] + vp[0][1]), (vp[1][3] + vp[1][1]),
                          (vp[2][3] + vp[2][1]), (vp[3][3] + vp[3][1]) };
        glm::vec4 top   { (vp[0][3] - vp[0][1]), (vp[1][3] - vp[1][1]),
                          (vp[2][3] - vp[2][1]), (vp[3][3] - vp[3][1]) };
        glm::vec4 near  { (vp[0][3] + vp[0][2]), (vp[1][3] + vp[1][2]),
                          (vp[2][3] + vp[2][2]), (vp[3][3] + vp[3][2]) };
        glm::vec4 far   { (vp[0][3] - vp[0][2]), (vp[1][3] - vp[1][2]),
                          (vp[2][3] - vp[2][2]), (vp[3][3] - vp[3][2]) };

        return std::array<glm::vec4, 6>
        {
            left   / glm::length(glm::vec3(left  )),
            right  / glm::length(glm::vec3(right )),
            bottom / glm::length(glm::vec3(bottom)),
            top    / glm::length(glm::vec3(top   )),
            near   / glm::length(glm::vec3(near  )),
            far    / glm::length(glm::vec3(far   )),
        };
    }
}

namespace rke
{
// public
    SceneRenderer::SceneRenderer(glm::vec4 col) : clear_color_(col)
    {
        scene_fbo_ = FrameBuffer::create ({
            .attachment_spec {
                { Texture::Format::RGBA16F, clear_color_ },
                { Texture::Format::R32I, -1 },
                { Texture::Format::DEPTH24_STENCIL8 }
            }
        });
    }

    const Texture2D* SceneRenderer::on_render(const Scene* scene,
        const glm::mat4& vp, glm::vec3 pos)
    {
        if(!scene) { scene_fbo_->clear(); return nullptr; }

        scene_fbo_->clear_to_upload([this, scene, &vp, pos]()
        {
            Renderer2D::begin_camera(vp);
            render_scene(scene, vp, pos);
        });

        return post_processor_.process(scene_fbo_->get_texture(0));
    }

    const Texture2D* SceneRenderer::on_render_runtime(const Scene* scene)
    {
        if(!scene) { scene_fbo_->clear(); return nullptr; }

        scene_fbo_->clear_to_upload([this, scene]()
        {
            Entity master_cam{ scene->get_master_camera() };
            // Use Projection Matrix to Render
            if(master_cam.valid())
            {
                const auto& tc{ master_cam.get<TransformComponent>() };
                const auto& proj{ master_cam.get<CameraComponent>().camera.get_proj() };
                glm::mat4 transform{ tc.get_transform() };

                auto vp{ proj * glm::inverse(transform) };
                Renderer2D::begin_camera(vp);
                render_scene(scene, vp, tc.position);
            }
        });

        return post_processor_.process(scene_fbo_->get_texture(0));
    }

    const Texture2D* SceneRenderer::cam_demo_render(const Scene* scene, Entity cam_demo)
    {
        if(!scene) { scene_fbo_->clear(); return nullptr; }

        scene_fbo_->clear_to_upload([this, cam_demo, scene]()
        {
            if(cam_demo.valid() && cam_demo.has<CameraComponent>())
                cam_demo_target_ = cam_demo;
            if(cam_demo_target_.valid() && cam_demo_target_.has<CameraComponent>())
            {
                const auto& tc{ cam_demo_target_.get<TransformComponent>() };
                const glm::mat4& projection
                    { cam_demo_target_.get<CameraComponent>().camera.get_proj() };
                glm::mat4 transform{ tc.get_transform() };

                auto vp{ projection * glm::inverse(transform) };
                Renderer2D::begin_camera(vp);
                render_scene(scene, vp, tc.position);
            }
        });

        return post_processor_.process(scene_fbo_->get_texture(0));
    }

    void SceneRenderer::on_viewport_resized(uint32 w, uint32 h)
    {
        scene_fbo_->resize(w, h);
        post_processor_.on_viewport_resized(w, h);
    }

    int SceneRenderer::get_hovering_id(int mouse_x, int mouse_y)
    {
        // make sure border checked
        if(scene_fbo_ && mouse_x >= 0 && mouse_y >= 0
        && mouse_x < scene_fbo_->get_specification().width
        && mouse_y < scene_fbo_->get_specification().height)
            return scene_fbo_->read_pixel(1, mouse_x, mouse_y);
        return -1;
    }

// private
    void SceneRenderer::draw_renderable(const Scene* scene, const Renderable& renderable)
    {
        const auto& tc{ scene->registry_->
            get<TransformComponent>(static_cast<entt::entity>(renderable.entity)) };
        if(scene->registry_->all_of<SpriteComponent>
            (static_cast<entt::entity>(renderable.entity)))
        {
            const auto& sc{ scene->registry_->
                get<SpriteComponent>(static_cast<entt::entity>(renderable.entity)) };
            const auto& ta{ sc.texture }; // texture asset
            auto* tex{ AssetsManager::get_asset<Texture2D>(ta.handle) };
            Renderer2D::draw_quad ({
                .position{ tc.position }, .rotation{ tc.rotation },
                .size{ tc.size.x, tc.size.y }, .color { sc.color },
                .uv_coords{ math::calc_uv(tex, ta.cell_coords, ta.cell_pixels, ta.cell_counts) },
                .tiling_factor{ ta.tiling_factor }, .texture{ tex },
                .entity_id{ static_cast<int>(renderable.entity) } // EDITOR ONLY
            });
        }
    //  else if(scene->registry_->all_of<MeshComponent>(renderable.entity)) {...}
        else {
            const auto& tag_com { scene->registry_->
                get<TagComponent >(static_cast<entt::entity>(renderable.entity)) };
            const auto& uuid_com{ scene->registry_->
                get<UUIDComponent>(static_cast<entt::entity>(renderable.entity)) };
            CORE_ASSERT(false, u8"SceneRenderer: Entity '{}'(UUID:{}) "
                u8"is not renderable!", tag_com.tag, uuid_com.uuid.value());
        }
    }

    void SceneRenderer::render_scene(const Scene* scene,
        const glm::mat4& view_projection, glm::vec3 cam_position)
    {
    // frustum culling
        auto planes{ get_planes_normal(view_projection) };

    // sort in-sight entities
        opaque_queue_.clear();
        cutout_queue_.clear();
        transparent_queue_.clear();

        auto view{ scene->registry_->view<TransformComponent, SpriteComponent>() };
        for(entt::entity entity : view) {
            const auto& tc{ view.get<TransformComponent>(entity) };
            if(should_cull(tc.position, glm::vec2(tc.size), planes)) continue;

            auto& sc{ view.get<SpriteComponent>(entity) };
            auto& ta{ sc.texture }; // texture asset
            if(ta.has_asset() && !ta.is_loaded()) {
                ta.handle = AssetsManager::load_asset(ta.uuid);
                if(auto* tex{ AssetsManager::get_asset<Texture2D>(ta.handle) })
                    ta.cell_pixels = glm::vec2(tex->get_width(), tex->get_height());
                else {
                    CORE_ERROR(u8"SceneRenderer: UUID '{}' invalid! "
                        u8"It's been reset to 0!", ta.uuid.value());
                    ta.uuid = { 0 }; // uuid been reset here!
                }
            }

            uint32 handle{ static_cast<uint32>(entity) };
            switch(sc.blending_mode)
            {
            case SpriteComponent::BlendingMode::Opaque:
                opaque_queue_.emplace_back(handle, sc.rendering_layer); break;
            case SpriteComponent::BlendingMode::Cutout:
                cutout_queue_.emplace_back(handle, sc.rendering_layer); break;
            case SpriteComponent::BlendingMode::Transparent: // Depends on camera distance
            {
                glm::vec3 dist{ tc.position - cam_position };
                float dist_sqr{ glm::dot(dist, dist) };
                transparent_queue_.emplace_back(handle, sc.rendering_layer, dist_sqr);
            } break;
            default: break;
            }
        }

        std::sort(opaque_queue_.begin(), opaque_queue_.end(),
            [](const Renderable& lhs, const Renderable& rhs)
                { return lhs.layer < rhs.layer; });
        std::sort(cutout_queue_.begin(), cutout_queue_.end(),
            [](const Renderable& lhs, const Renderable& rhs)
                { return lhs.layer < rhs.layer; });
        std::sort(transparent_queue_.begin(), transparent_queue_.end());

    //  auto view{ scene->registry_->view<TransformComponent, MeshComponent>() };
    //  for(auto entity : view) {...}

    // render in-sight entities(in sorted order)
        Renderer2D::begin_scene();

        for(const auto& renderable : opaque_queue_)
            draw_renderable(scene, renderable);
        for(const auto& renderable : cutout_queue_)
            draw_renderable(scene, renderable);

        Renderer2D::end_scene();

        if(!transparent_queue_.empty())
        {
            RenderCommand::set_depth_write(false);
            RenderCommand::blend_func_transparent();

            Renderer2D::begin_scene();

            for(const auto& renderable : transparent_queue_)
                draw_renderable(scene, renderable);

            Renderer2D::end_scene();

            RenderCommand::blend_func_default();
            RenderCommand::set_depth_write(true);
        }
    }
}
