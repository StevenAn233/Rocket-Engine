export module ScriptLoader;

import Path;
import String;

export namespace rke
{
    class ScriptLoader
    {
    public:
        static bool load_dylib(const Path& dir, const String& name);
        static void unload_all_dylibs();
        static void delete_temp_files(const Path& dir);
    };
}
