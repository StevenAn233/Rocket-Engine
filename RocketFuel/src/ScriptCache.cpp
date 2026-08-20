// script-dylib side

#ifdef RKE_PLATFORM_WINDOWS
    #define RKE_GLUE_API __declspec(dllexport)
#else
    #define RKE_GLUE_API
#endif

#include <vector>
#include <memory>

import ScriptRegistry;

namespace
{
    struct ScriptEntry
    {
        const char8_t* name;
        void* (*constructor)();
    };

    static std::vector<ScriptEntry> s_script_entries{}; // cache
}

namespace rke::glue
{
    void push_script_entry(const char8_t* name, void* (*constructor)())
        { s_script_entries.emplace_back(name, constructor); }

    extern "C" RKE_GLUE_API bool register_scripts(ScriptRegistry* reg)
    {
        if(!reg) return false;
        for(ScriptEntry entry : s_script_entries)
            reg->register_script(entry.name, entry.constructor);
        return true;
    }
}
