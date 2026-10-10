module;
module SceneRenderer;

import Log;
import Project;
import Renderer;
import Application;
import RenderCommand;
import Components;
import Texture;
import Plane;

namespace
{
    using namespace rke;
    using Frustum = std::array<PlaneBasis, 6>;

    static bool should_cull(glm::vec3 pos, const glm::mat3& basis,
        glm::vec3 half_size, const Frustum& frustum)
    {
        for(const auto& plane : frustum)
        {
            const glm::vec3 normal{ plane.get_normal() };
            const float support {
                half_size.x * std::abs(glm::dot(basis[0], normal))
              + half_size.y * std::abs(glm::dot(basis[1], normal))
              + half_size.z * std::abs(glm::dot(basis[2], normal))
            };
            if(plane.signed_distance(pos) + support < 0.0f) return true;
        }
        return false;
    }

    static Frustum get_frustum(const glm::mat4& vp)
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

        return Frustum {
            PlaneBasis(left  ), PlaneBasis(right ),
            PlaneBasis(bottom), PlaneBasis(top   ),
            PlaneBasis(near  ), PlaneBasis(far   ),
        };
    }

    static std::pair<Texture*, GTextureSettings> get_texture(AssetsManager& am, Entity entity)
    {
        if(!entity.is_valid() || !entity.has<SpriteComponent>()) return { nullptr, {} };
        auto& sc{ entity.get_mut<SpriteComponent>() };
        if(entity.has<TextureComponent>())
        {
            auto& txc{ entity.get_mut<TextureComponent>() };
            if(txc.tex_uuid.empty()) return { nullptr, {} };

            auto [handle, _]{ am.resolve(txc.resolved_tex, txc.tex_uuid) };
            if(handle == asset_handle_null) return { nullptr, {} };

            Texture* tex{ am.get_asset<Texture>(handle) };
            CORE_ASSERT(tex, u8"SceneRenderer: Texture null!");

            sc.uv_scale = sprite::compute_uv_scale
                (txc.cell_size, tex->get_width(), tex->get_height());
            sc.uv_offset = sprite::compute_uv_offset(txc.cell_coords, sc.uv_scale);
            return { tex, txc.gtex_settings };
        }
        // else if: if have both(not gonna happen normally); use texture
        else if(entity.has<AnimatorComponent>()) 
        {
            const auto& ac{ entity.get<AnimatorComponent>() };
            if(!am.is_handle_valid(ac.curr_tex_handle)) return { nullptr, {} };

            Texture* tex{ am.get_asset<Texture>(ac.curr_tex_handle) };
            CORE_ASSERT(tex, u8"SceneRenderer: Texture null!");

            sc.uv_scale = sprite::compute_uv_scale
                (ac.curr_cell_size, tex->get_width(), tex->get_height());
            sc.uv_offset = sprite::compute_uv_offset(ac.curr_cell_coords, sc.uv_scale);
            return { tex, ac.gtex_settings };
        }
        return { nullptr, {} };
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

    PostProcessEffect* SceneRenderer::push_effect(Scope<PostProcessEffect> effect)
        { return post_processor_.push_effect(std::move(effect)); }

    Scope<PostProcessEffect> SceneRenderer::pop_effect()
        { return post_processor_.pop_effect(); }

    void SceneRenderer::refresh_post_processor_shaders()
        { post_processor_.refresh_all_effect_shaders(); }
    
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

    const GTexture2D* SceneRenderer::render(Entity camera)
    {
        if(camera.is_valid() && camera.has<CameraComponent>())
        {
            const auto& proj{ camera.get<CameraComponent>().camera.get_proj() };
            glm::mat4 view_proj{ proj * glm::inverse(camera.get_world_transform().matrix) };
            return render(camera.get_owner(), view_proj, camera.compute_centre());
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
    void SceneRenderer::draw_entity(AssetsManager& manager,
        const Scene* scene, EntityHandle handle)
    {
        entt::entity entity{ static_cast<entt::entity>(handle) };
        entt::registry& reg{ *(scene->registry_) };

        if(reg.all_of<TransformComponent, SpriteComponent>(entity))
        {
            const Entity self{ scene->get_entity(handle) };
            auto [tex, gtex_settings]{ get_texture(manager, self) };
            auto& sc{ reg.get<SpriteComponent>(entity) };
            GTexture* gtex{ tex ? tex->get_gtexture(gtex_settings) : nullptr };
            context_->renderer().push(sc.quad, gtex, RenderProps
            {
            // the accumulated transform, so a child follows its parent
                .transform{ self.get_world_transform().matrix },
                .uv_offset{ sc.uv_offset },
                .uv_scale { sc.uv_scale  },
                .color{ sc.color }, .entity_handle{ handle }
            });
        } else {
            const auto& ic{ reg.get<IdentityComponent>(entity) };
            CORE_ASSERT(false, u8"SceneRenderer: Entity '{}' "
                u8"is not renderable!", String(ic.tag));
        }
    }

    void SceneRenderer::render_scene(const Scene* scene, const glm::mat4& vp, glm::vec3 cam_pos)
    {
        Frustum frustum{ get_frustum(vp) };
        AssetsManager& assets_manager{ scene->get_owner()->get_assets_manager_mut() };
        
        transparent_queue_.clear();
        context_->renderer().begin_scene();
        auto view{ scene->registry_->view<TransformComponent, SpriteComponent>() };
        for(entt::entity ent : view)
        {
            const auto& sc{ view.get<SpriteComponent>(ent) };
            if(sc.color.a < 0.01f) continue;

            const EntityHandle handle{ static_cast<EntityHandle>(ent) };
            const Entity entity{ scene->get_entity(handle) };
            const glm::mat4 world_mat{ entity.get_world_transform().matrix };
            const glm::vec3 pos{ glm::vec3(world_mat * glm::vec4(sc.quad->get_centre(), 1.0f)) };
            const glm::mat3 basis{ glm::mat3(world_mat) };
            const glm::vec3 half_size{ 0.5f * sc.quad->get_size() };
            if(should_cull(pos, basis, half_size, frustum)) continue;

            switch(sc.blending_mode)
            {
            case BlendingMode::Opaque:
                draw_entity(assets_manager, scene, handle); break;
            case BlendingMode::Transparent:
            {
                float dist{ glm::length(pos - cam_pos) };
                transparent_queue_.emplace_back(handle, sc.rendering_layer, dist);
            } break;
            default: break;
            }
        }
        context_->renderer().end_scene();
        if(transparent_queue_.empty()) return;

        std::sort(transparent_queue_.begin(), transparent_queue_.end());
        
        app().render_command().set_depth_write(false);
        context_->renderer().begin_scene();

        for(const auto& renderable : transparent_queue_)
            draw_entity(assets_manager, scene, renderable.handle);

        context_->renderer().end_scene();
        app().render_command().set_depth_write(true);
    }
}
