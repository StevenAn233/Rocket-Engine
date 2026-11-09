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
#include "rke_fuel.h"

import ScriptAccess;
import EffectAccess;

namespace {
    static std::vector<rke::ScriptEntry>& get_script_entires_cache()
    {
        static std::vector<rke::ScriptEntry> s_script_entries_cache{};
        return s_script_entries_cache;
    }

    static std::vector<rke::EffectEntry>& get_effect_entires_cache()
    {
        static std::vector<rke::EffectEntry> s_effect_entries_cache{};
        return s_effect_entries_cache;
    }
}

namespace rke::fuel
{
    void push_script_entry(const char8_t* type_name,
        void* (*constructor)(), void* (*deep_copier)(const void*))
    {
        if(!type_name) return;
        get_script_entires_cache().emplace_back
        (
            std::bit_cast<ScriptType>(type_name),
            ScriptRecipe(constructor, deep_copier)
        );
    }

    void push_effect_entry(const char8_t* type_name, void* (*constructor)())
    {
        if(!type_name) return;
        get_effect_entires_cache().emplace_back
        (
            std::bit_cast<EffectType>(type_name),
            constructor
        );
    }
}

extern "C"
{
    RKE_GLUE_API void rke_fuel_all_scripts(rke::ScriptsView* out)
    {
        if(!out) return;
        const auto& cache{ get_script_entires_cache() };
        out->data = cache.data();
        out->size = cache.size();
    }

    RKE_GLUE_API void rke_fuel_all_effects(rke::EffectsView* out)
    {
        if(!out) return;
        const auto& cache{ get_effect_entires_cache() };
        out->data = cache.data();
        out->size = cache.size();
    }
}
