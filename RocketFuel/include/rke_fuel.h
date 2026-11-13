#ifndef RKE_FUEL_H
#define RKE_FUEL_H

import String;

namespace rke::fuel
{
    void push_script_entry(const char8_t* type_name,
        void* (*constructor)(), void* (*deep_copier)(const void*));

    void push_effect_entry(const char8_t* type_name,
        void* (*constructor)());

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

    template<typename RkeEffect>
    struct EffectRegistrar
    {
        EffectRegistrar(const char8_t* type_name)
        {
            push_effect_entry(type_name, []()
                -> void* { return new RkeEffect(); });
        }
    };
}

// put this into class public area
#define RKE_SCRIPT_TYPE_INTERFACE \
    static const char8_t* get_static_type_name(); \
    const char8_t* get_type_name() const override

// put this into cpp file
#define RKE_REGISTER_SCRIPT(RkeScript) \
    const char8_t* RkeScript##::get_static_type_name() \
        { return u8## #RkeScript; } \
    const char8_t* RkeScript##::get_type_name() const \
        { return get_static_type_name(); } \
    static ::rke::fuel::ScriptRegistrar<RkeScript> \
        s_rke_reg_##RkeScript{ RkeScript##::get_static_type_name() }

// put this into class public area
#define RKE_EFFECT_TYPE_INTERFACE \
    static const char8_t* get_static_type_name(); \
    const char8_t* get_type_name() const override

// put this into cpp file
#define RKE_REGISTER_EFFECT(RkeEffect) \
    const char8_t* RkeEffect##::get_static_type_name() \
        { return u8## #RkeEffect; } \
    const char8_t* RkeEffect##::get_type_name() const \
        { return get_static_type_name(); } \
    static ::rke::fuel::EffectRegistrar<RkeEffect> \
        s_rke_reg_##RkeEffect{ RkeEffect##::get_static_type_name() }

#endif // RKE_FUEL_H
