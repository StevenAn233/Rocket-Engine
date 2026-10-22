module;
module RkDylibLoader;

import :Base;
import Log;

namespace rke
{
    RkDylibLoader::RkDylibLoader(Path dir, String name)
        : dylib_dir_(std::move(dir)), dylib_name_(std::move(name)) {}
}

#ifdef RKE_PLATFORM_WINDOWS
import :Windows;

namespace rke
{
    Scope<RkDylibLoader> RkDylibLoader::create(Path dir, String name)
    {
        return create_scope<WindowsRkDylibLoader>
            (std::move(dir), std::move(name));
    }
}
#endif
