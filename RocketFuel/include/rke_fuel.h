#ifndef RKE_FUEL_H
#define RKE_FUEL_H

namespace rke::fuel
{
    void push_script_entry(const char8_t* type_name,
        void* (*constructor)(), void* (*deep_copier)(const void*));

    template<typename RkeScript>
    struct ScriptRegistrar
    {
        ScriptRegistrar(const char8_t* type_name)
        {
            push_script_entry(type_name,
                []() -> void* { return new RkeScript(); },
                [](const void* other) -> void*
                {
                    if(!other) return nullptr;
                    return new RkeScript
                        (*reinterpret_cast<const RkeScript*>(other));
                }
            );
        }
    };
}

// must be string literals, so pointers are fine here
#define RKE_REGISTER_SCRIPT(RkeScript) \
static rke::fuel::ScriptRegistrar<RkeScript> s_rke_reg_##RkeScript{ u8## #RkeScript }

#endif
