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
    struct HModuleDeleter { void operator()(HMODULE h) const; };
    using DylibData = std::unique_ptr<std::remove_pointer_t<HMODULE>, HModuleDeleter>;

    class WindowsScriptDylibLoader : public ScriptDylibLoader
    {
    public:
        WindowsScriptDylibLoader() = default;
        ~WindowsScriptDylibLoader() override = default;

        WindowsScriptDylibLoader(const WindowsScriptDylibLoader&) = delete;
        WindowsScriptDylibLoader& operator=(const WindowsScriptDylibLoader&) = delete;
        WindowsScriptDylibLoader(WindowsScriptDylibLoader&&) = default;
        WindowsScriptDylibLoader& operator=(WindowsScriptDylibLoader&&) = default;

        bool load_dylib(const Path& dir, const String& name) override;
        void unload_all_dylibs() override;
        void delete_temp_files(const Path& dir) override;
    private:
        std::vector<DylibData> dylib_stack_{};
        uint32 reload_count_{};
    };
}
