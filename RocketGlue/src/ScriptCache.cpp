#ifdef RKE_PLATFORM_WINDOWS
    #define RKE_GLUE_API __declspec(dllexport)
#else
    #define RKE_GLUE_API
#endif

#include <vector>
#include <memory>

import ScriptRegistry;

namespace {
    struct ScriptEntry
    {
        const char8_t* name;
        void* (*constructor)();
    };

    static std::vector<ScriptEntry>& get_cache()
    {
        static std::vector<ScriptEntry> cache{};
        return cache;
    }
}

namespace rke::glue
{
    void internal_add_script(const char8_t* name, void* (*constructor)())
        { get_cache().emplace_back(name, constructor); }

    extern "C" RKE_GLUE_API void register_scripts()
    {
        for(auto& entry : get_cache())
            ScriptRegistry::register_script(entry.name, entry.constructor);
    }
}
