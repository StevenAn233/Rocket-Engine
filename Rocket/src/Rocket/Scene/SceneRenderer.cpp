module;
module SceneRenderer;

import Log;
import Project;
import Renderer;
import Application;
import RenderCommand;
import Components;
import Texture;

namespace {
    using namespace rke;

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

    static Texture* get_texture(AssetsManager& am, SpriteComponent& sc)
    {
        if(sc.resolved_uuid != sc.tex_uuid || !am.is_handle_valid(sc.tex_handle))
        {
            sc.tex_handle = am.load_asset(sc.tex_uuid);
            sc.resolved_uuid = sc.tex_uuid;
        }
        return am.get_asset<Texture>(sc.tex_handle);
    }
}

namespace rke
{
// public
    SceneRenderer::SceneRenderer(Window* context, glm::vec4 col)
        : context_(context), clear_color_(col)
    {
        CORE_ASSERT(context_, u8"SceneRenderer: Window context null!");
        scene_fbo_ = FrameBuffer::create ({
            .attachment_spec {
                { GTexture::Format::RGBA16F, clear_color_ },
                { GTexture::Format::R32I, -1 },
                { GTexture::Format::DEPTH24_STENCIL8 }
            }
        });
    }

    void SceneRenderer::add_effect(Scope<PostProcessEffect> effect)
        { post_processor_.add_effect(std::move(effect)); }
    
    const GTexture2D* SceneRenderer::render(const Scene* scene, const glm::mat4& vp, glm::vec3 pos)
    {
        if(!scene) { scene_fbo_->clear(); return nullptr; }
        scene_fbo_->clear_to_upload([this, scene, &vp, pos]()
        {
            context_->renderer().begin_camera(vp);
            render_scene(scene, vp, pos);
        });
        return post_processor_.process(scene_fbo_->get_gtexture_attached(0));
    }

    const GTexture2D* SceneRenderer::render(const Scene* scene, Entity camera)
    {
        if(camera.valid() && camera.belongs_to(scene) && camera.has<CameraComponent>())
        {
            const auto& tc{ camera.get<TransformComponent>() };
            const auto& proj{ camera.get<CameraComponent>().camera.get_proj() };

            glm::mat4 view_proj{ proj * glm::inverse(tc.get_transform()) };
            return render(scene, view_proj, tc.translation + glm::vec3(0.0f));
        }
        return nullptr;
    }

    void SceneRenderer::on_viewport_resized(uint32 w, uint32 h)
    {
        scene_fbo_->resize(w, h);
        post_processor_.on_viewport_resized(w, h);
    }

    int SceneRenderer::get_hovering_id(int mouse_x, int mouse_y)
    {
        // check border
        if(scene_fbo_ && mouse_x >= 0 && mouse_y >= 0
        && mouse_x < scene_fbo_->get_specification().width
        && mouse_y < scene_fbo_->get_specification().height)
            return scene_fbo_->read_pixel(1, mouse_x, mouse_y);
        return -1;
    }

    void SceneRenderer::clean_up()
    {
        scene_fbo_->clear_pbo();
        scene_fbo_->clear();
        post_processor_.clean_up();
    }
    
// private
    void SceneRenderer::draw_entity(AssetsManager& manager, const Scene* scene, uint32 handle)
    {
        entt::entity entity{ static_cast<entt::entity>(handle) };
        entt::registry& reg{ *(scene->registry_) };
        const auto& tc{ reg.get<TransformComponent>(entity) };
        if(reg.all_of<SpriteComponent>(entity))
        {
            auto& sc{ reg.get<SpriteComponent>(entity) };
            Texture* tex{ get_texture(manager, sc) };
            GTexture* gtex{ tex ? tex->get_gtexture(sc.gtex_settings) : nullptr };
            context_->renderer().push(sc.quad, gtex, RenderProps
            {
                .transform{ tc.get_transform() },
                .uv_offset{ sc.uv_offset },
                .uv_scale { sc.uv_scale  },
                .color{ sc.color },
                .entity_id{ handle }
            });
        } else {
            const auto& ic{ reg.get<IdentityComponent>(entity) };
            CORE_ASSERT(false, u8"SceneRenderer: Entity '{}'(UUID:{}) "
                u8"is not renderable!", String(ic.tag), ic.uuid.value());
        }
    }

    void SceneRenderer::render_scene(const Scene* scene,
        const glm::mat4& vp, glm::vec3 cam_pos)
    {
    // frustum culling
        auto planes{ get_planes_normal(vp) };

    // sort in-sight entities
        opaque_queue_.clear();
        cutout_queue_.clear();
        transparent_queue_.clear();

        AssetsManager& assets_manager{ scene->get_owner()->get_assets_manager_mut() };
        auto view{ scene->registry_->view<TransformComponent, SpriteComponent>() };
        for(entt::entity entity : view)
        {
            const auto& tc{ view.get<TransformComponent>(entity) };
            auto& sc{ view.get<SpriteComponent>(entity) };
            if(sc.color.a < 0.01f) continue;

            glm::vec3 pos { tc.translation + sc.quad->get_centre() };
            glm::vec3 size{ tc.scale * sc.quad->get_size() };
            if(should_cull(pos, size, planes)) continue;

            uint32 handle{ static_cast<uint32>(entity) };
            switch(sc.blending_mode)
            {
            case SpriteComponent::BlendingMode::Opaque:
                opaque_queue_.emplace_back(handle, sc.rendering_layer, 0.0f); break;
            case SpriteComponent::BlendingMode::Cutout:
                cutout_queue_.emplace_back(handle, sc.rendering_layer, 0.0f); break;
            case SpriteComponent::BlendingMode::Transparent: // Depends on camera distance
            {
                glm::vec3 dist{ pos - cam_pos };
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

    // render entities
        context_->renderer().begin_scene();

        for(const auto& renderable : opaque_queue_)
            draw_entity(assets_manager, scene, renderable.handle);
        for(const auto& renderable : cutout_queue_)
            draw_entity(assets_manager, scene, renderable.handle);

        context_->renderer().end_scene();

        if(transparent_queue_.empty()) return;
        
        app().render_command().set_depth_write(false);
        app().render_command().blend_func_transparent();

        context_->renderer().begin_scene();

        for(const auto& renderable : transparent_queue_)
            draw_entity(assets_manager, scene, renderable.handle);

        context_->renderer().end_scene();

        app().render_command().blend_func_default();
        app().render_command().set_depth_write(true);
    }
}
