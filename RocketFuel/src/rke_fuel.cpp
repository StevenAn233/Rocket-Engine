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

import ScriptAccess;
import ScriptRegistry;

namespace {
    using namespace rke;
    using ScriptEntry = std::pair<ScriptType, ScriptRecipe>;

    static std::vector<ScriptEntry>& get_scripts_entires_cache()
    {
        static std::vector<ScriptEntry> s_script_entries_cache{};
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
    RKE_GLUE_API void rke_fuel_register_scripts(rke::ScriptRegistry* reg)
    {
        if(!reg) return;
        for(const auto& [type, recipe] : get_scripts_entires_cache())
            reg->register_script(type, recipe);
    }
}
