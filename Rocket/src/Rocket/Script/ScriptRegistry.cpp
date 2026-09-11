module;
module ScriptRegistry;

import Log;

namespace rke
{
    ScriptRegistry::ScriptRegistry(Scope<ScriptDylib> dylib)
        : dylib_(std::move(dylib))
    {
        if(!dylib_) return;
        if(!dylib_->valid()) { dylib_.reset(); return; }
        ScriptsRegistar registar{ dylib_->get_scripts_registar() };
        CORE_ASSERT(registar, u8"ScriptRegistry: Registar null!");
        registar(this);
    }

    ScriptRegistry::~ScriptRegistry() { clear(); }

    void ScriptRegistry::register_script(ScriptType type, ScriptConstructor func)
    {
        CORE_ASSERT(!has_script_type(type),
            u8"ScriptRegistry: Script has already been registered!");
        script_types_.push_back(type);
        script_constructors_.emplace(static_cast<uintptr>(type), func);
    }

    void ScriptRegistry::clear()
    {
        script_types_.clear();
        script_constructors_.clear();
        CORE_INFO(u8"ScriptRegistry: All registered scripts cleared.");
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
