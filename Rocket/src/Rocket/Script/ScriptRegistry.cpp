module;
module ScriptRegistry;

namespace rke
{
    void ScriptRegistry::register_script(const char8* name, ScriptConstructor func)
    {
        script_types_.push_back(name);
        String name_string{ name };
        CORE_ASSERT(!has_script(name_string),
            u8"ScriptRegistry: Script has already been registered!");
        script_constructors_.emplace(std::move(name_string), func);
    }

    Scope<Script> ScriptRegistry::construct_script(const String& name)
    {
        if(name.empty()) return nullptr;
        auto it{ script_constructors_.find(name) };
        if(it != script_constructors_.end()) // will always be Script*
            return Scope<Script>(static_cast<Script*>(it->second()));
        CORE_ERROR(u8"ScriptRegistry: Script '{}' is not registered!", name);
        return nullptr;
    }

    bool ScriptRegistry::has_script(const String& name) const
        { return script_constructors_.contains(name); }

    void ScriptRegistry::clear()
    {
        script_constructors_.clear();
        CORE_INFO(u8"ScriptRegistry: All registered scripts cleared.");
    }
}
