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
    class RKE_API RkDylib
    {
    public:
        friend class Project;

        virtual ~RkDylib() = default;

        Scope<Script> construct_script(ScriptType type) const;
        Scope<Script> deep_copy_script(const Script* other) const;
        bool has_script_type(ScriptType type) const;
    
        String get_script_name(ScriptType type) const;
        ScriptType get_script_type(const String& type_name) const; // expensive
        bool has_script(const String& type_name) const; // expensive

        Scope<PostProcessEffect> construct_effect(EffectType type) const;
        bool has_effect_type(EffectType type) const;
    
        String get_effect_name(EffectType type) const;
        EffectType get_effect_type(const String& type_name) const; // expensive
        bool has_effect(const String& type_name) const; // expensive

        inline Project* get_owner() const { return owner_; }
        inline const std::vector<ScriptType>& get_script_types() const { return script_types_; }
        inline const std::vector<EffectType>& get_effect_types() const { return effect_types_; }
    protected:
        RkDylib() {}
    protected:
        Project* owner_{}; // set by Project

        std::vector<ScriptType> script_types_{};
        std::unordered_map<uintptr, ScriptRecipe> script_recipes_{};
        std::vector<EffectType> effect_types_{};
        std::unordered_map<uintptr, EffectRecipe> effect_recipes_{};
    };
}
