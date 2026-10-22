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
        WindowsRkDylib(const Path& path);
        ~WindowsRkDylib() override;

        bool is_valid() const override;
        ScriptsRegistar get_scripts_registar() const override;
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

        Scope<RkDylib> load_dylib() const override;
    private:
        void delete_temp_files();
    private:
        mutable uint32 reload_count_{};
    };
}
