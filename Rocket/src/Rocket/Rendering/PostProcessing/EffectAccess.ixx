export module EffectAccess;

import Types;
import String;

export namespace rke
{
    enum class EffectType : uintptr {};
    constexpr EffectType effect_type_null{ static_cast<EffectType>(0) };

    struct EffectEntry
    {
        EffectType type;
        void* (*constructor)(const String& name);
    };

    struct EffectsView
    {
        const EffectEntry* data{};
        uint64 size{};
    };
}
