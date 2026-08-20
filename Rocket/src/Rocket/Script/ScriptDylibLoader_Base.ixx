export module ScriptDylibLoader:Base;

import Path;
import String;
import HeapManager;
import ScriptRegistry;

export namespace rke
{
    using RegisterScriptsFunc = bool(*)(ScriptRegistry*);

    class ScriptDylibLoader
    {
    public:
        ScriptDylibLoader(Path dir, String name);
        virtual ~ScriptDylibLoader() = default;

        virtual bool load_dylib() = 0;
        RegisterScriptsFunc get_register_scripts_func() const { return func_; }

        static Scope<ScriptDylibLoader> create(Path dir, String name);
    protected:
        Path dylib_dir_{};
        String dylib_name_{};
        RegisterScriptsFunc func_{ nullptr };
    };
}
