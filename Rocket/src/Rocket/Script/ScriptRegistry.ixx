module;

#include <unordered_map>
#include "rke_macros.h"

export module ScriptRegistry;

import Types;
import Log;
import String;
import Script;
import HeapManager;

export namespace rke
{
    class RKE_API ScriptRegistry
    {
    public:
        using ScriptConstructor = void* (*)();
        using ScriptMap = std::unordered_map<String, ScriptConstructor>;

        static void register_script(const char8* name, ScriptConstructor func);
        // will always be string literals(if not directly called)

        static Scope<Script> construct_through_name(const String& name);
        static const ScriptMap& get() { return get_script_map(); }
        static void clear();
    private:
        static ScriptMap& get_script_map();
    };
}
