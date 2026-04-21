export module ScriptLoader;

import Path;

export namespace rke
{
    class ScriptLoader
    {
    public:
        static bool load_script_dll(const Path& dll_path);
        static void unload_all();
    };
}
