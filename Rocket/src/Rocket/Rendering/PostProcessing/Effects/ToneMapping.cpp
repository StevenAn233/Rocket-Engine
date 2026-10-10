module;
module ToneMapping;

import BindingPoint;
import GBuffers;
import AssetsManager;
import Application;
import RenderCommand;
import FileUtils;
import GShader;
import Log;

namespace rke
{
    ToneMapping::ToneMapping(String name) : PostProcessEffect(std::move(name))
    {
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        shader_ = create_scope<Shader>(file::assets_dir() / u8"shaders" / u8"tone_mapping.rkshdr");
    }

    bool ToneMapping::apply(const GTexture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;

        GShader* gshader{ shader_ ? shader_->get_gshader() : nullptr };
        if(!gshader) {
            CORE_ERROR(u8"ToneMapping: Shader unavailable, the frame is left untouched!");
            return false; // keep the source texture
        }

        destination->clear_to_upload([this, source, gshader]()
        {
            source->bind(BindingPoint::Sampler2D_0);
            ubo_->bind(BindingPoint::UBO_PostProcess);

            gshader->bind();
            app().render_command().draw_quad();
            gshader->unbind();
        });
        return true;
    }

    void ToneMapping::set_uniform(const Uniforms& uniforms)
        { ubo_->set_data(&uniforms, sizeof(Uniforms)); }
}
