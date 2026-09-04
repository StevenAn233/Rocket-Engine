export module ScriptAccess;

import Types;

export namespace rke
{
    enum class ScriptType : uintptr {};
    constexpr ScriptType script_type_null{ static_cast<ScriptType>(0) };
    using ScriptConstructor = void*(*)();
}
