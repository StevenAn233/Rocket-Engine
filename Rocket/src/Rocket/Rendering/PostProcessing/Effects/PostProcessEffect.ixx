module;

#include <memory>
#include <utility>
#include <functional>
#include <unordered_map>
#include "rke_macros.h"

export module PostProcessEffect;

import HeapManager;
import Path;
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

        PostProcessEffect(String name, std::function<bool()> extra = nullptr);
        virtual ~PostProcessEffect() = default;
        virtual Category get_category() const { return Category::Standard; }

        inline const String& get_name() const { return name_; }
        inline bool is_enabled() const { return enabled_inner_ && enabled_extra_(); }

        // if returns true , use destination->get_gtexture_attached();
        // if returns false, use (original)source
        virtual bool apply(const GTexture2D* source, FrameBuffer* destination) = 0;

        void serialize_to(ConfigWriter& writer) const;
        void deserialize_from(const ConfigReader& reader);
    protected:
        virtual void on_serialize(ConfigWriter& writer) const {}
        virtual void on_deserialize(const ConfigReader& reader) {}

        virtual void on_imgui_render();
        virtual void on_viewport_resized(uint32 w, uint32 h) {}; // for client framebuffer
    private:
        String name_;
        bool enabled_inner_{ true };
        std::function<bool()> enabled_extra_{};
    };
}
