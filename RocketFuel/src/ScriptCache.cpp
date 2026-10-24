// script-dylib side

#ifdef RKE_PLATFORM_WINDOWS
    #define RKE_GLUE_API __declspec(dllexport)
#else
    #define RKE_GLUE_API
#endif

#include <bit>
#include <vector>
#include <memory>
#include <utility>

import Script;
import ScriptAccess;
import ScriptRegistry;

namespace rke::glue
{
    using ScriptEntry = std::pair<const char8_t*, ScriptRecipe>;

    static std::vector<ScriptEntry>& get_entires_cache()
    {
        static std::vector<ScriptEntry> s_script_entries_cache{};
        return s_script_entries_cache;
    }

    void push_script_entry(const char8_t* name,
        void* (*constructor)(), void* (*deep_copier)(const void*))
    {
        get_entires_cache().emplace_back
            (name, ScriptRecipe(constructor, deep_copier));
    }

    extern "C" RKE_GLUE_API void register_scripts(ScriptRegistry* reg)
    {
        if(!reg) return;
        for(auto [name, recipe] : get_entires_cache())
            reg->register_script(std::bit_cast<ScriptType>(name), recipe);
    }
}
