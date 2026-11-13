module;

#include <bit>
#include "rke_macros.h"
namespace rke { class Project; }

export module PostProcessEffect;

import Types;
import String;
import GTexture;
import FrameBuffer;
import ConfigProxy;
import EffectAccess;

export namespace rke
{
    class RKE_API PostProcessEffect
    {
    public:
        friend class PostProcessor;
        friend class RkDylib;

        enum class Category
        {
            Standard, // e.g. Bloom, ToneMapping, Vignette
            Helper,   // e.g. Outline, Grid, Gizmo
            System    // e.g. FXAA, SMAA
        };

        PostProcessEffect(String name);
        virtual ~PostProcessEffect() = default;
        virtual Category get_category() const { return Category::Standard; }

        inline const String& get_name() const { return name_; }

        // if returns true , use destination->get_gtexture_attached();
        // if returns false, use (original)source
        virtual bool apply(const GTexture2D* source, FrameBuffer* destination) = 0;
        virtual bool is_enabled() const;

        void serialize_to(ConfigWriter& writer) const;
        void deserialize_from(const ConfigReader& reader);

        virtual const char8_t* get_type_name() const = 0;
        inline EffectType get_effect_type() const
            { return std::bit_cast<EffectType>(get_type_name()); }
    protected:
        virtual void on_serialize(ConfigWriter& writer) const {}
        virtual void on_deserialize(const ConfigReader& reader) {}

        virtual void on_imgui_render();
        virtual void on_viewport_resized(uint32 w, uint32 h) {}

        inline Project* get_context() const { return context_; }
    private:
        Project* context_{}; // set by RkDylib
        String name_;
        bool enabled_{ true };
    };

    template<typename T>
    inline EffectType effect_type_of()
    {
        const char8_t* name{ T::get_static_type_name() };
        return std::bit_cast<EffectType>(name);
    }

    template<typename T>
    requires std::is_base_of_v<PostProcessEffect, T>
    inline T* effect_cast(PostProcessEffect* effect)
    {
        if(!effect) return nullptr;
        const EffectType expected{ effect_type_of<T>() };
        if(expected == effect_type_null) return nullptr;
        if(effect->get_effect_type() != expected) return nullptr;
        return static_cast<T*>(effect); // the type matched, so this is exact
    }

    template<typename T>
    requires std::is_base_of_v<PostProcessEffect, T>
    inline const T* script_cast(const PostProcessEffect* effect)
    {
        if(!effect) return nullptr;
        const EffectType expected{ effect_type_of<T>() };
        if(expected == effect_type_null) return nullptr;
        if(effect->get_effect_type() != expected) return nullptr;
        return static_cast<const T*>(effect);
    }
}
