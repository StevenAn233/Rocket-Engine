module;
module FXAAEffect;

namespace rke
{
    static PostProcessEffect::TypeIdentity s_rke_ppetid
    {
        .type{ u8"FxaaEffect" },
        .shaders{{ u8"Fxaa", Shader(file::assets_dir() / u8"shaders" / u8"fxaa.rkshdr") }}
    };

    FXAAEffect::FXAAEffect(String name, std::function<bool()> func)
        : PostProcessEffect(&s_rke_ppetid, std::move(name), std::move(func))
        { ubo_ = UniformBuffer::create(sizeof(Uniforms)); }

    bool FXAAEffect::apply(const GTexture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;

        auto* shader { get_shader(u8"Fxaa") };
        auto* gshader{ shader ? shader->get_gshader() : nullptr };
        if(!gshader) {
            CORE_ERROR(u8"FXAAEffect: Shader unavailable, the frame is left untouched!");
            return false; // keep the source texture
        }

        destination->clear();
        destination->upload([this, source, gshader]()
        {
            source->bind(BindingPoint::Sampler2D_0);
            ubo_->bind(BindingPoint::UBO_PostProcess);

            gshader->bind();
            app().render_command().draw_quad();
            gshader->unbind();
        });
        return true;
    }

    void FXAAEffect::on_viewport_resized(uint32 w, uint32 h)
    {
        Uniforms uni{ glm::vec2(1.0f / w, 1.0f / h) };
        ubo_->set_data(&uni, sizeof(Uniforms));
    }
}
