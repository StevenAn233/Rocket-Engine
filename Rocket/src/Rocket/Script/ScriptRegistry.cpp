module;
module ScriptRegistry;

import Log;

namespace rke
{
    void ScriptRegistry::register_script(uintptr type_id, ScriptConstructor func)
    {
        ScriptType type{ static_cast<ScriptType>(type_id) };
        CORE_ASSERT(!has_script_type(type), u8"ScriptRegistry: Script has already been registered!");
        script_types_.push_back(type);
        script_constructors_.emplace(type_id, func);
    }

    Scope<Script> ScriptRegistry::construct_script(ScriptType type)
    {
        // ONLY do address compare, not string compare!
        if(type == script_type_null) return nullptr;
        auto it{ script_constructors_.find(static_cast<uintptr>(type)) };
        if(it != script_constructors_.end())
            return Scope<Script>(reinterpret_cast<Script*>(std::invoke(it->second)));
        return nullptr;
    }

    bool ScriptRegistry::has_script_type(ScriptType type) const
        { return script_constructors_.contains(static_cast<uintptr>(type)); }

    String ScriptRegistry::get_script_name(ScriptType type) const
        { return String(std::bit_cast<const char8*>(type)); }

    ScriptType ScriptRegistry::get_script_type(const String& name) const
    {
        for(ScriptType type : script_types_)
            if(String(std::bit_cast<const char8*>(type)) == name) return type;
        return script_type_null;
    }

    bool ScriptRegistry::has_script(const String& name) const
        { return static_cast<bool>(get_script_type(name)); }

    void ScriptRegistry::clear()
    {
        script_types_.clear();
        script_constructors_.clear();
        CORE_INFO(u8"ScriptRegistry: All registered scripts cleared.");
    }
}
