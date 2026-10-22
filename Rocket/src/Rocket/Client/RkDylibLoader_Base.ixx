module;

namespace rke { class ScriptRegistry; }

export module RkDylibLoader:Base;

import Path;
import String;
import HeapManager;

export namespace rke
{
    using ScriptsRegistar = void(*)(ScriptRegistry*);

    class RkDylib
    {
    public:
        virtual ~RkDylib() = default;
        
        virtual bool is_valid() const = 0;
        virtual ScriptsRegistar get_scripts_registar() const = 0;
    protected:
        RkDylib() {};
    };

    class RkDylibLoader
    {
    public:
        RkDylibLoader(Path dir, String name);
        virtual ~RkDylibLoader() = default;

        virtual Scope<RkDylib> load_dylib() const = 0;

        static Scope<RkDylibLoader> create(Path dir, String name);
    protected:
        Path dylib_dir_{};
        String dylib_name_{};
    };
}
