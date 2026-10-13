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
    private:
        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        void refresh_shader() override;
    private:
        Uniforms uniforms_{};
        Scope<Shader> shader_{};
        Ref<UniformBuffer> ubo_{};
    };
}
