module;

#include "rke_macros.h"

export module ToneMapping;

import PostProcessEffect;
import Texture;
import FrameBuffer;

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

        bool apply(const Texture2D* source, FrameBuffer* destination) override;
        void set_uniform(const Uniforms& uniforms);
    };
}
