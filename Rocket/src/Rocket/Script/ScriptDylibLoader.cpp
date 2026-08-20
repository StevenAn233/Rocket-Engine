module;
module ScriptDylibLoader;

import :Base;

#ifdef RKE_PLATFORM_WINDOWS
import :Windows;

namespace rke
{
    Scope<ScriptDylibLoader> ScriptDylibLoader::create()
        { return create_scope<WindowsScriptDylibLoader>(); }
}
#endif
