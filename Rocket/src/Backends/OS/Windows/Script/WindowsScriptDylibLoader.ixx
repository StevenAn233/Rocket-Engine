module;

#include <type_traits>
#include <vector>
#include <memory>
#include <windows.h>

export module ScriptDylibLoader:Windows;

import :Base;
import Types;
import Path;
import String;

namespace rke
{
    struct WindowsDylib : public ScriptDylib
    {
    public:
        WindowsDylib(const Path& path);
        ~WindowsDylib() override;

        bool valid() const override;
        ScriptsRegistar get_scripts_registar() const override;
    private:
        HMODULE handle_{};
    };

    class WindowsScriptDylibLoader : public ScriptDylibLoader
    {
    public:
        WindowsScriptDylibLoader(Path dir, String name)
            : ScriptDylibLoader(std::move(dir), std::move(name)) {}
        ~WindowsScriptDylibLoader() override;

        WindowsScriptDylibLoader(const WindowsScriptDylibLoader&) = delete;
        WindowsScriptDylibLoader& operator=(const WindowsScriptDylibLoader&) = delete;
        WindowsScriptDylibLoader(WindowsScriptDylibLoader&&) = default;
        WindowsScriptDylibLoader& operator=(WindowsScriptDylibLoader&&) = default;

        Scope<ScriptDylib> load_dylib() const override;
    private:
        void delete_temp_files();
    private:
        mutable uint32 reload_count_{};
    };
}
