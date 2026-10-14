module;
module ToneMapping;

import Log;
import Layout;
import BindingPoint;
import AssetsManager;
import Application;
import RenderCommand;
import FileUtils;
import GShader;

namespace rke
{
    static PostProcessEffect::TypeIdentity s_rke_ppetid
    {
        .type{ u8"ToneMapping" },
        .shaders{{ u8"Tone Mapping", Shader(file::assets_dir() / u8"shaders" / u8"tone_mapping.rkshdr") }}
    };

    ToneMapping::ToneMapping(String name)
        : PostProcessEffect(&s_rke_ppetid, std::move(name))
    {
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    void ToneMapping::set_exposure(float exposure)
    {
        uniforms_.exposure = exposure;
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    void ToneMapping::set_gamma(float gamma)
    {
        uniforms_.gamma = gamma;
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    bool ToneMapping::apply(const GTexture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;

        auto* shader{ get_shader(u8"Tone Mapping") };
        GShader* gshader{ shader ? shader->get_gshader() : nullptr };
        if(!gshader) {
            CORE_ERROR(u8"ToneMapping: Shader unavailable, the frame is left untouched!");
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
    
    void ToneMapping::serialize_to(ConfigWriter& writer) const
    {
        PostProcessEffect::serialize_to(writer);
        writer.write(u8"Exposure", uniforms_.exposure);
        writer.write(u8"Gamma", uniforms_.gamma);
    }

    void ToneMapping::deserialize_from(const ConfigReader& reader)
    {
        PostProcessEffect::deserialize_from(reader);
        uniforms_.exposure = reader.get_at(u8"Exposure", 1.0f);
        uniforms_.gamma = reader.get_at(u8"Gamma", 2.2f);
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    void ToneMapping::on_imgui_render()
    {
        PostProcessEffect::on_imgui_render();

        bool modified{ false };
        if(layout::drag_float_control<u8"Exposure">(uniforms_.exposure, 0.01f, 1.0f))
            modified = true;
        if(layout::drag_float_control<u8"Gamma">(uniforms_.gamma, 0.01f, 2.2f))
            modified = true;
        if(modified) ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }
}
