module;

namespace rke { class ScriptRegistry; }

export module ScriptDylibLoader:Base;

import Path;
import String;
import HeapManager;

export namespace rke
{
    using ScriptsRegistar = void(*)(ScriptRegistry*);

    class ScriptDylib
    {
    public:
        ScriptDylib() = default;
        virtual ~ScriptDylib() = default;
        
        virtual bool valid() const = 0;
        virtual ScriptsRegistar get_scripts_registar() const = 0;
    };

    class ScriptDylibLoader
    {
    public:
        ScriptDylibLoader(Path dir, String name);
        virtual ~ScriptDylibLoader() = default;

        virtual Scope<ScriptDylib> load_dylib() const = 0;

        static Scope<ScriptDylibLoader> create(Path dir, String name);
    protected:
        Path dylib_dir_{};
        String dylib_name_{};
    };
}
