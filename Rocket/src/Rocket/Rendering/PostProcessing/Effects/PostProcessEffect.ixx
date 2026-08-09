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
import Texture;
import Buffers;
import FrameBuffer;
import ConfigProxy;

export namespace rke
{
    class RKE_API PostProcessEffect
    {
    public:
        enum class Category
        {
            Standard, // e.g. Bloom, ToneMapping, Vignette
            Helper,   // e.g. Outline, Grid, Gizmo
            System    // e.g. FXAA, SMAA
        };

        friend class PostProcessor;
        friend class std::default_delete<PostProcessEffect>;

        const String& get_name() const { return name_; }
        bool enabled() const { return enabled_situation_(); }

        virtual bool apply(const Texture2D* source, FrameBuffer* destination) = 0;
        // if returns true , use destination->get_texture();
        // if returns false, use (original)source

        virtual void serialize_to(ConfigWriter& writer) const {}
        virtual void deserialize_from(const ConfigReader& reader) {}
        virtual Category get_category() const { return Category::Standard; }
    protected:
        PostProcessEffect(String name, std::function<bool()> func = nullptr);
        virtual ~PostProcessEffect() = default;
        
        virtual void on_viewport_resized(uint32 w, uint32 h) {};
    protected:
        String name_;
        Ref<UniformBuffer> ubo_{};
        Scope<Shader> shader_{};
    private:
        std::function<bool()> enabled_situation_{};
    };
}
