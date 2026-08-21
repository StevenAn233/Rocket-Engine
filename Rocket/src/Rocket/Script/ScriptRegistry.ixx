module;

#include <vector>
#include <unordered_map>
#include "rke_macros.h"

export module ScriptRegistry;

import Types;
import String;

export namespace rke
{
    class ScriptRegistry
    {
    public:
        using ScriptConstructor = void*(*)();

        ScriptRegistry() = default;
        ~ScriptRegistry() = default;

        // name will always be string literals(if not directly called)
        RKE_API void register_script(const char8* name, ScriptConstructor func);
        RKE_API void clear();

        RKE_API void* construct_script(const String& name);
        RKE_API bool has_script(const String& name) const;

        inline const std::vector<const char8*>& get_script_types() const { return script_types_; }
    private:
        std::vector<const char8*> script_types_{};
        std::unordered_map<String, ScriptConstructor> script_constructors_{};
    };
}
