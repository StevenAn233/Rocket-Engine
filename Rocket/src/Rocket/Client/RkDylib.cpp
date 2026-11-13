module;
module RkDylib;

import Log;

namespace rke
{
    Scope<Script> RkDylib::construct_script(ScriptType type) const
    {
        // only do address compare, not string compare
        if(type == script_type_null) return nullptr;
        const auto it{ script_recipes_.find(static_cast<uintptr>(type)) };
        if(it != script_recipes_.end())
        {
            Scope<Script> script{ reinterpret_cast<Script*>
                (std::invoke(it->second.constructor)) };
            return script;
        }
        return nullptr;
    }

    Scope<Script> RkDylib::deep_copy_script(const Script* other) const
    {
        if(!other) return nullptr;
        const ScriptType script_type{ other->get_script_type() };
        if(script_type == script_type_null)
        {
            CORE_WARN(u8"RkDylib: Other script type invaild!");
            return nullptr;
        }

        const auto it{ script_recipes_.find
            (static_cast<uintptr>(script_type)) };
        if(it != script_recipes_.end())
        {
            Scope<Script> script{ reinterpret_cast<Script*>
                (std::invoke(it->second.deep_copier, other)) };
            return script;
        }
        return nullptr;
    }

    bool RkDylib::has_script_type(ScriptType type) const
        { return script_recipes_.contains(static_cast<uintptr>(type)); }

    String RkDylib::get_script_name(ScriptType type) const
    {
        if(!has_script_type(type)) return {};
        return String(std::bit_cast<const char8*>(type));
    }

    ScriptType RkDylib::get_script_type(const String& type_name) const
    {
        for(ScriptType type : script_types_) // string compare
            if(String(std::bit_cast<const char8*>(type)) == type_name) return type;
        return script_type_null;
    }

    bool RkDylib::has_script(const String& type_name) const
        { return static_cast<bool>(get_script_type(type_name)); }

    Scope<PostProcessEffect> RkDylib::construct_effect(EffectType type, const String& name) const
    {
        // only do address compare, not string compare
        if(type == effect_type_null) return nullptr;
        const auto it{ effect_recipes_.find(static_cast<uintptr>(type)) };
        if(it != effect_recipes_.end())
        {
            Scope<PostProcessEffect> effect{ reinterpret_cast
                <PostProcessEffect*>(std::invoke(it->second, name)) };
            effect->context_ = owner_;
            return effect;
        }
        return nullptr;
    }

    bool RkDylib::has_effect_type(EffectType type) const
        { return effect_recipes_.contains(static_cast<uintptr>(type)); }

    String RkDylib::get_effect_name(EffectType type) const
    {
        if(!has_effect_type(type)) return {};
        return String(std::bit_cast<const char8*>(type));
    }

    EffectType RkDylib::get_effect_type(const String& type_name) const
    {
        for(EffectType type : effect_types_) // string compare
            if(String(std::bit_cast<const char8*>(type)) == type_name) return type;
        return effect_type_null;
    }

    bool RkDylib::has_effect(const String& type_name) const
        { return static_cast<bool>(get_effect_type(type_name)); }
}
