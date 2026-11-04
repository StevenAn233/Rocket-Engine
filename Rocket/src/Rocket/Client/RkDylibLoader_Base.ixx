module;

#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <concepts>
#include <functional>
#include "rke_macros.h"
namespace rke { class ScriptRegistry; }

export module RkDylibLoader:Base;

import Log;
import Path;
import String;
import HeapManager;
import Types;
import Script;
import ScriptAccess;

export namespace rke
{
    class RkDylib
    {
    public:
        virtual ~RkDylib() = default;

        RKE_API Scope<Script> construct_script(ScriptType type) const;
        RKE_API Scope<Script> deep_copy_script(const Script* other) const;
        RKE_API bool has_script_type(ScriptType type) const;
    
        RKE_API String get_script_name(ScriptType type) const;
        RKE_API ScriptType get_script_type(const String& name) const; // expensive
        RKE_API bool has_script(const String& name) const; // expensive

        inline const std::vector<ScriptType>& get_script_types() const { return script_types_; }
    protected:
        RkDylib() {};
    protected:
        std::vector<ScriptType> script_types_{};
        std::unordered_map<uintptr, ScriptRecipe> script_recipes_{};
    };

    class RkDylibLoader
    {
    public:
        RkDylibLoader(Path dir, String name);
        virtual ~RkDylibLoader() = default;

        virtual Scope<RkDylib> load_fresh() = 0;

        static Scope<RkDylibLoader> create(Path dir, String name);
    protected:
        Path dylib_dir_{};
        String dylib_name_{};

        Scope<RkDylib> current_dylib_{};
        uint32 reload_count_{};
    };
}
