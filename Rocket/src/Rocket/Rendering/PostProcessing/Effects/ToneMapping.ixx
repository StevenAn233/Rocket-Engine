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

        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        void set_uniform(const Uniforms& uniforms);
    private:
        void refresh_shader() override;
    private:
        Scope<Shader> shader_{};
        Ref<UniformBuffer> ubo_{};
    };
}
