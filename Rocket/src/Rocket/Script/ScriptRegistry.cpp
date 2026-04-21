module;
module ScriptRegistry;

namespace rke
{
    ScriptRegistry::ScriptMap& ScriptRegistry::get_script_map()
    {
        static ScriptRegistry::ScriptMap s_script_map{};
        return s_script_map;
    }

    void ScriptRegistry::register_script(const char8* name, ScriptConstructor func)
    {
        String name_string{ name };
        CORE_ASSERT(get_script_map().find(name_string) == get_script_map().end(),
            u8"ScriptRegistry: Script has already been registered!");
        get_script_map().emplace(std::move(name_string), func);
    }

    Scope<Script> ScriptRegistry::construct_through_name(const String& name)
    {
        if(name.empty()) return nullptr;
        auto it{ get_script_map().find(name) };
        if(it != get_script_map().end()) // will always be Script*
            return Scope<Script>(static_cast<Script*>(it->second()));
        CORE_ERROR(u8"ScriptRegistry: Script '{}' is not registered!", name);
        return nullptr;
    }

    void ScriptRegistry::clear()
    {
        get_script_map().clear();
        CORE_INFO(u8"ScriptRegistry: All registered scripts cleared.");
    }
}
