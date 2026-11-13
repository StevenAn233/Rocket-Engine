export module EffectAccess;

import Types;
import String;

export namespace rke
{
    enum class EffectType : uintptr {};
    constexpr EffectType effect_type_null{ static_cast<EffectType>(0) };

    struct EffectRecipe
    {
        void* (*constructor)();
    };

    struct EffectEntry
    {
        EffectType type;
        EffectRecipe recipe;
    };

    struct EffectsView
    {
        const EffectEntry* data{};
        uint64 size{};
    };
}
