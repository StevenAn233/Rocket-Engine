// script-dylib side

#ifdef RKE_PLATFORM_WINDOWS
    #define RKE_GLUE_API __declspec(dllexport)
#else
    #define RKE_GLUE_API
#endif

#include <bit>
#include <vector>
#include <memory>

import Script;
import ScriptRegistry;

namespace
{
    struct ScriptEntry
    {
        const char8_t* name;
        void* (*constructor)();
    };

    static std::vector<ScriptEntry>& get_entires_cache()
    {
        static std::vector<ScriptEntry> s_script_entries_cache{};
        return s_script_entries_cache;
    }
}

namespace rke::glue
{
    void push_script_entry(const char8_t* name, void* (*constructor)())
        { get_entires_cache().emplace_back(name, constructor); }

    extern "C" RKE_GLUE_API bool register_scripts(ScriptRegistry* reg)
    {
        if(!reg) return false;
        for(ScriptEntry entry : get_entires_cache())
            reg->register_script(std::bit_cast<ScriptType>(entry.name), entry.constructor);
        return true;
    }
}
