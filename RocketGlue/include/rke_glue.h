#pragma once

namespace rke::glue {
    void internal_add_script(const char8_t* name, void* (*constructor)());
}

template<typename S>
struct RKEScriptRegistrar
{
    RKEScriptRegistrar(const char8_t* name) {
        rke::glue::internal_add_script
           (name, []() -> void* { return new S{}; });
    }
};

#define RKE_REGISTER_SCRIPT(Type) \
    static RKEScriptRegistrar<Type> s_Reg_##Type(u8## #Type)
// must be string literals, so pointers are good here
