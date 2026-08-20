export module ScriptDylibLoader:Base;

import Path;
import String;
import HeapManager;

export namespace rke
{
    class ScriptDylibLoader
    {
    public:
        virtual ~ScriptDylibLoader() = default;

        virtual bool load_dylib(const Path& dir, const String& name) = 0;
        virtual void unload_all_dylibs() = 0;
        virtual void delete_temp_files(const Path& dir) = 0;

        static Scope<ScriptDylibLoader> create();
    };
}
