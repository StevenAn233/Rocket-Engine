module;
module ScriptDylibLoader;

import :Base;
import Log;

namespace rke
{
    ScriptDylibLoader::ScriptDylibLoader(Path dir, String name)
        : dylib_dir_(std::move(dir)), dylib_name_(std::move(name)) {}
}

#ifdef RKE_PLATFORM_WINDOWS
import :Windows;

namespace rke
{
    Scope<ScriptDylibLoader> ScriptDylibLoader::create(Path dir, String name)
    {
        return create_scope<WindowsScriptDylibLoader>
            (std::move(dir), std::move(name));
    }
}
#endif
