module;

#include <memory>
#include <vector>
#include <unordered_map>
#include "rke_macros.h"

export module ScriptRegistry;

import Types;
import String;
import Script;
import HeapManager;

export namespace rke
{
    class ScriptRegistry
    {
    public:
        ScriptRegistry() = default;
        ~ScriptRegistry() = default;

        // name will always be string literals(if not directly called)
        RKE_API void register_script(ScriptType type, ScriptConstructor func);
        RKE_API void clear();

        RKE_API Scope<Script> construct_script(ScriptType type);
        RKE_API bool has_script_type(ScriptType type) const;
    
        RKE_API String get_script_name(ScriptType type) const;
        RKE_API ScriptType get_script_type(const String& name) const; // expensive
        RKE_API bool has_script(const String& name) const; // expensive

        inline const std::vector<ScriptType>& get_script_types() const { return script_types_; }
    private:
        std::vector<ScriptType> script_types_{};
        std::unordered_map<uintptr, ScriptConstructor> script_constructors_{};
    };
}
