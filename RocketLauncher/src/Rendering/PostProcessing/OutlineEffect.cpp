module;
module OutlineEffect;

import Layout;

namespace rke
{
// public
    OutlineEffect::OutlineEffect(String name, Window* context,
        std::function<bool()> enabled_situation,
        std::function<Entity()> target_getter)
        : PostProcessEffect(std::move(name), std::move(enabled_situation))
        , context_(context), target_getter_(std::move(target_getter))
    {
        CORE_ASSERT(context_, u8"OutlineEffect: Context window null!");
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        outline_fbo_ = FrameBuffer::create
            ({ .attachment_spec{{ Texture::Format::R8, 0.0f }} });
        shader_ = Shader::create(file::assets_dir() / u8"shaders" / u8"composite.rkshdr");
    }

    bool OutlineEffect::apply(const Texture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;
        Entity target{ target_getter_() };
        if(!target.valid()) return false;
        outline_fbo_->clear_to_upload([this, target]()
        {
            if(target.has<SpriteComponent>())
            {
                context_->renderer_2d().begin_scene();

                const auto& tc{ target.get<TransformComponent>() };
                context_->renderer_2d().draw_quad
                ({
                    .position{ tc.position },
                    .rotation{ tc.rotation },
                    .size	 { tc.size	   },
                    .color   { glm::vec4(1.0f) },
                });

                context_->renderer_2d().end_scene();
            }
        //  else if(target_.has<MeshComponent>()) {...}
        });

        auto* silhouette{ outline_fbo_->get_texture() };
        if(!silhouette) return false;

        destination->clear_to_upload([this, source, silhouette]()
        {
            ubo_->bind(BindingPoint::UBO_PostProcess);
            source->bind(BindingPoint::Sampler2D_0);
            silhouette->bind(BindingPoint::Sampler2D_1);

            shader_->bind();
            app().render_command().draw_quad();
            shader_->unbind();
        });
        return true;
    }

    void OutlineEffect::serialize_to(ConfigWriter& writer) const
    {
        writer.begin_map(get_name());

        writer.write(u8"Color", math::linear_to_srgb(uniforms_.outline_color));
        writer.write(u8"Thickness", uniforms_.thickness);

        writer.end_map();
    }

    void OutlineEffect::deserialize_from(const ConfigReader& reader)
    {
        auto config{ reader.get_child(get_name()) };
        if(!config || !config->is_map())
            { CORE_ERROR(u8"OutlineEffect: Wrong yaml format!"); return; }
        set_color(config->get_at(u8"Color", glm::vec4{}));
        set_thickness(config->get_at(u8"Thickness", 1.0f));
    }

    void OutlineEffect::set_samples(uint32 samples)
        { outline_fbo_->set_samples(samples); }

    void OutlineEffect::set_color(glm::vec4 color)
    {
        uniforms_.outline_color = math::srgb_to_linear(color);
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    void OutlineEffect::set_thickness(float thickness)
    {
        uniforms_.thickness = thickness;
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    void OutlineEffect::on_viewport_resized(uint32 w, uint32 h) { outline_fbo_->resize(w, h); }
}
