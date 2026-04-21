module;
module ToneMapping;

import BindingPoint;
import Buffers;
import AssetsManager;
import RenderCommand;
import Application;

namespace rke
{
    ToneMapping::ToneMapping(String name) : PostProcessEffect(std::move(name))
    {
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        shader_ = Shader::create(Application::get()
            .asset_path(Path(u8"shaders") / u8"tone_mapping.rkshdr"));
    }

    bool ToneMapping::apply(const Texture2D* source, FrameBuffer* destination)
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

    void ToneMapping::set_uniform(const Uniforms& uniforms)
        { ubo_->set_data(&uniforms, sizeof(Uniforms)); }
}
