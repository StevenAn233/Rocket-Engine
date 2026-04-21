module;
module OutlineEffect;

import Layout;

namespace rke
{
// public
    OutlineEffect::OutlineEffect(String name)
        : PostProcessEffect(std::move(name))
    {
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        outline_fbo_ = FrameBuffer::create
            ({ .attachment_spec{{ Texture::Format::R8, 0.0f }} });
        shader_ = Shader::create(Application::get()
            .asset_path(Path(u8"shaders") / u8"composite.rkshdr"));
    }

    bool OutlineEffect::apply(const Texture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination || !target_.valid()) return false;

        outline_fbo_->clear_to_upload([this]()
        {
            if(target_.has<SpriteComponent>())
            {
                Renderer2D::begin_scene();

                const auto& tc{ target_.get<TransformComponent>() };
                Renderer2D::draw_quad
                ({
                    .position{ tc.position },
                    .rotation{ tc.rotation },
                    .size	 { tc.size	   },
                    .color   { glm::vec4(1.0f) },
                });

                Renderer2D::end_scene();
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
            RenderCommand::draw_quad();
            shader_->unbind();
        });
        return true;
    }

    void OutlineEffect::on_imgui_render()
    {
        float outline_thickness{ uniforms_.thickness };
        if(layout::drag_float_control(u8"Thickness",
           outline_thickness, 0.01f, 1.0f, glm::vec2(0.0f, 2.0f)))
            { set_thickness(outline_thickness); }
        // change color...
    }

    void OutlineEffect::serialize_to(ConfigWriter* writer) const
    {
        if(!writer) {
            CORE_ERROR(u8"OutlineEffect: Writer null!");
            return;
        }
        writer->begin_map(get_name());

        writer->write(u8"Enabled", enabled());
        writer->write(u8"Color", math::linear_to_srgb(uniforms_.outline_color));
        writer->write(u8"Thickness", uniforms_.thickness);

        writer->end_map();
    }

    void OutlineEffect::deserialize_from(const ConfigReader* reader)
    {
        if(!reader) {
            CORE_ERROR(u8"OutlineEffect: Reader null!");
            return;
        }
        auto config{ reader->get_child(get_name()) };
        if(!config || !config->is_map()) {
            CORE_ERROR(u8"OutlineEffect: Wrong yaml format!");
            return;
        }
        set_enabled(config->get_at(u8"Enabled", true));
        set_color(config->get_at(u8"Color", glm::vec4{}));
        set_thickness(config->get_at(u8"Thickness", 1.0f));
    }

    void OutlineEffect::set_target(Entity target) { target_ = target; }

    void OutlineEffect::set_samples(uint32 samples)
        { outline_fbo_->set_samples(samples); }

    void OutlineEffect::on_viewport_resized(uint32 w, uint32 h)
        { outline_fbo_->resize(w, h); }

// private
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
}
