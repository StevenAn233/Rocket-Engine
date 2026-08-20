#pragma once

namespace rke::glue {
    void push_script_entry(const char8_t* name, void* (*constructor)());
}

template<typename RKEScript>
struct RKEScriptRegistrar
{
    RKEScriptRegistrar(const char8_t* name)
    {
        rke::glue::push_script_entry
           (name, []() -> void* { return new RKEScript{}; });
    }
};

#define RKE_REGISTER_SCRIPT(RKEScript) \
static RKEScriptRegistrar<RKEScript> s_reg_##RKEScript{ u8## #RKEScript }
// must be string literals, so pointers are fine here
