module;
module FXAAEffect;

namespace rke
{
    FXAAEffect::FXAAEffect(String name, std::function<bool()> extra)
        : PostProcessEffect(std::move(name), std::move(extra))
        { ubo_ = UniformBuffer::create(sizeof(Uniforms)); }

    bool FXAAEffect::apply(const GTexture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;

        auto* gshader{ get_shader().get_gshader() };
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

// static
    Shader& FXAAEffect::get_shader()
    {
        static Shader s_shader{ file::assets_dir()
            / u8"shaders" / u8"fxaa.rkshdr" };
        return s_shader;
    }
}
