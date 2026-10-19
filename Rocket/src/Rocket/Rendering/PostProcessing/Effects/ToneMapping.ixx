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

        ToneMapping(String name, std::function<bool()> extra = nullptr);
        void set_exposure(float exposure);
        void set_gamma(float gamma);

        static void refresh_shader() { get_shader().clear_gshaders(); }
    private:
        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        void serialize_to(ConfigWriter& writer) const override;
        void deserialize_from(const ConfigReader& reader) override;
        void on_imgui_render() override;

        static Shader& get_shader();
    private:
        Uniforms uniforms_{};
        Ref<UniformBuffer> ubo_{};
    };
}
