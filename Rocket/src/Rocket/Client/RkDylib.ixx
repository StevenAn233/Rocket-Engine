module;

#include <vector>
#include <unordered_map>
#include "rke_macros.h"

export module RkDylib;

import String;
import HeapManager;
import Types;
import Script;
import ScriptAccess;
import PostProcessEffect;
import EffectAccess;

export namespace rke
{
    class RkDylib
    {
    public:
        virtual ~RkDylib() = default;

        RKE_API Scope<Script> construct_script(ScriptType type) const;
        RKE_API Scope<Script> deep_copy_script(const Script* other) const;
        RKE_API bool has_script_type(ScriptType type) const;
    
        RKE_API String get_script_name(ScriptType type) const;
        RKE_API ScriptType get_script_type(const String& type_name) const; // expensive
        RKE_API bool has_script(const String& type_name) const; // expensive

        RKE_API Scope<PostProcessEffect> construct_effect(EffectType type, const String& name) const;
        RKE_API bool has_effect_type(EffectType type) const;
    
        RKE_API String get_effect_name(EffectType type) const;
        RKE_API EffectType get_effect_type(const String& type_name) const; // expensive
        RKE_API bool has_effect(const String& type_name) const; // expensive

        inline const std::vector<ScriptType>& get_script_types() const { return script_types_; }
    protected:
        RkDylib() {};
    protected:
        std::vector<ScriptType> script_types_{};
        std::unordered_map<uintptr, ScriptRecipe> script_recipes_{};
        std::vector<EffectType> effect_types_{};
        std::unordered_map<uintptr, void* (*)(const String&)> effect_recipes_{};
    };
}
