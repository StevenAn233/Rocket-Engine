module;

#include <type_traits>
#include <vector>
#include <memory>
#include <windows.h>

export module RkDylibLoader:Windows;

import :Base;
import Types;
import Path;
import String;

namespace rke
{
    struct WindowsRkDylib : public RkDylib
    {
    public:
        friend class WindowsRkDylibLoader;

        WindowsRkDylib(const Path& path);
        ~WindowsRkDylib() override;
    private:
        HMODULE handle_{};
    };

    class WindowsRkDylibLoader : public RkDylibLoader
    {
    public:
        WindowsRkDylibLoader(Path dir, String name)
            : RkDylibLoader(std::move(dir), std::move(name)) {}
        ~WindowsRkDylibLoader() override;

        WindowsRkDylibLoader(const WindowsRkDylibLoader&) = delete;
        WindowsRkDylibLoader& operator=(const WindowsRkDylibLoader&) = delete;
        WindowsRkDylibLoader(WindowsRkDylibLoader&&) = default;
        WindowsRkDylibLoader& operator=(WindowsRkDylibLoader&&) = default;

        Scope<RkDylib> load_fresh() override;
    private:
        void prune_temp_files();
        void delete_temp_files();
    };
}
