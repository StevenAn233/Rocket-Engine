module;

#include <memory>
#include <utility>
#include <functional>
#include "rke_macros.h"

export module PostProcessEffect;

import Shader;
import HeapManager;
import Types;
import String;
import GTexture;
import GBuffers;
import FrameBuffer;
import ConfigProxy;

export namespace rke
{
    class RKE_API PostProcessEffect
    {
    public:
        friend class PostProcessor;

        enum class Category
        {
            Standard, // e.g. Bloom, ToneMapping, Vignette
            Helper,   // e.g. Outline, Grid, Gizmo
            System    // e.g. FXAA, SMAA
        };

        PostProcessEffect(String name, std::function<bool()> func = nullptr);
        virtual ~PostProcessEffect() = default;

        inline const String& get_name() const { return name_; }
        inline bool enabled() const { return enabled_situation_(); }
        

        virtual bool apply(const GTexture2D* source, FrameBuffer* destination) = 0;
        // if returns true , use destination->get_gtexture_attached();
        // if returns false, use (original)source

        virtual void serialize_to(ConfigWriter& writer) const {}
        virtual void deserialize_from(const ConfigReader& reader) {}
        virtual Category get_category() const { return Category::Standard; }
    protected:
        inline void refresh_shader() { shader_->clear_gshaders(); } // for post-processor
        virtual void on_viewport_resized(uint32 w, uint32 h) {};
    protected:
        String name_;
        Ref<UniformBuffer> ubo_{};
        Scope<Shader> shader_{};
    private:
        std::function<bool()> enabled_situation_{};
    };
}
