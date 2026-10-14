module;

#include "rke_macros.h"

export module ToneMapping;

import PostProcessEffect;
import GTexture;
import FrameBuffer;
import HeapManager;
import Shader;
import GBuffers;

export namespace rke
{
    class RKE_API ToneMapping : public PostProcessEffect
    {
    public:
        struct Uniforms
        {
            float exposure{ 1.0f };
            float gamma	  { 2.2f };
        };

        ToneMapping(String name);
        void set_exposure(float exposure);
        void set_gamma(float gamma);
    private:
        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        void refresh_shader() override;
        void serialize_to(ConfigWriter& writer) const override;
        void deserialize_from(const ConfigReader& reader) override;
        void on_imgui_render() override;
    private:
        Uniforms uniforms_{};
        Scope<Shader> shader_{};
        Ref<UniformBuffer> ubo_{};
    };
}
