module;
module FXAAEffect;

namespace rke
{
    FXAAEffect::FXAAEffect(String name, std::function<bool()> func)
        : PostProcessEffect(std::move(name), std::move(func))
    {
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        shader_ = Shader::create(file::assets_dir() / u8"shaders" / u8"fxaa.rkshdr");
    }

    bool FXAAEffect::apply(const Texture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;

        destination->clear_to_upload([this, source]()
        {
            source->bind(BindingPoint::Sampler2D_0);
            ubo_->bind(BindingPoint::UBO_PostProcess);

            shader_->bind();
            RenderCommand::draw_quad();
            shader_->unbind();
        });
        return true;
    }

    void FXAAEffect::set_uniform(const Uniforms& uniforms)
        { ubo_->set_data(&uniforms, sizeof(Uniforms)); }
}
