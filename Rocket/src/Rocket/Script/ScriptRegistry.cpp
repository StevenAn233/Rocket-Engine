module;
module ScriptRegistry;

import Log;
import Script;

namespace rke
{
    ScriptRegistry::~ScriptRegistry() { clear(); }

    void ScriptRegistry::register_script(ScriptType type, ScriptRecipe recipe)
    {
        CORE_ASSERT(!has_script_type(type),
            u8"ScriptRegistry: Script has already been registered!");
        script_types_.push_back(type);
        script_recipes_.emplace(static_cast<uintptr>(type), recipe);
    }

    void ScriptRegistry::clear()
    {
        script_types_.clear();
        script_recipes_.clear();
        CORE_INFO(u8"ScriptRegistry: All registered scripts cleared.");
    }

    Scope<Script> ScriptRegistry::construct_script(ScriptType type) const
    {
        // ONLY do address compare, not string compare!
        if(type == script_type_null) return nullptr;
        auto it{ script_recipes_.find(static_cast<uintptr>(type)) };
        if(it != script_recipes_.end())
            return Scope<Script>(reinterpret_cast<Script*>
                (std::invoke(it->second.constructor)));
        return nullptr;
    }

    Scope<Script> ScriptRegistry::deep_copy_script
        (ScriptType type, const Script* other) const
    {
        if(!other) return nullptr;
        if(type == script_type_null) return nullptr;
        auto it{ script_recipes_.find(static_cast<uintptr>(type)) };
        if(it != script_recipes_.end())
            return Scope<Script>(reinterpret_cast<Script*>
                (std::invoke(it->second.deep_copier, other)));
        return nullptr;
    }

    bool ScriptRegistry::has_script_type(ScriptType type) const
        { return script_recipes_.contains(static_cast<uintptr>(type)); }

    String ScriptRegistry::get_script_name(ScriptType type) const
    {
        if(!has_script_type(type)) return {};
        return String(std::bit_cast<const char8*>(type));
    }

    ScriptType ScriptRegistry::get_script_type(const String& name) const
    {
        for(ScriptType type : script_types_)
            if(String(std::bit_cast<const char8*>(type)) == name) return type;
        return script_type_null;
    }

    bool ScriptRegistry::has_script(const String& name) const
        { return static_cast<bool>(get_script_type(name)); }
}
