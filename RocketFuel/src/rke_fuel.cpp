// client-dylib side

#ifdef RKE_PLATFORM_WINDOWS
    #define RKE_GLUE_API __declspec(dllexport)
#else
    #define RKE_GLUE_API
#endif

#include <bit>
#include <vector>
#include <memory>
#include <utility>
#include "rke_scripting.h"

import ScriptAccess;

namespace {
    static std::vector<rke::ScriptEntry>& get_scripts_entires_cache()
    {
        static std::vector<rke::ScriptEntry> s_script_entries_cache{};
        return s_script_entries_cache;
    }
}

namespace rke::fuel
{
    void push_script_entry(const char8_t* type_name,
        void* (*constructor)(), void* (*deep_copier)(const void*))
    {
        if(!type_name) return;
        get_scripts_entires_cache().emplace_back
        (
            std::bit_cast<ScriptType>(type_name),
            ScriptRecipe(constructor, deep_copier)
        );
    }
}

extern "C"
{
    RKE_GLUE_API void rke_fuel_all_scripts(ScriptsView* out)
    {
        const auto& cache{ get_scripts_entires_cache() };
        out->data = cache.data();
        out->size = cache.size();
    }
}
