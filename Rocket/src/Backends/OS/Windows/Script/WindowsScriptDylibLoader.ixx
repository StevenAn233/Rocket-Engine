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
        WindowsScriptDylibLoader(Path dir, String name)
            : ScriptDylibLoader(std::move(dir), std::move(name)) {}
        ~WindowsScriptDylibLoader() override;

        WindowsScriptDylibLoader(const WindowsScriptDylibLoader&) = delete;
        WindowsScriptDylibLoader& operator=(const WindowsScriptDylibLoader&) = delete;
        WindowsScriptDylibLoader(WindowsScriptDylibLoader&&) = default;
        WindowsScriptDylibLoader& operator=(WindowsScriptDylibLoader&&) = default;

        bool load_dylib() override;
    private:    
        void clear_cache();
        void delete_temp_files();
    private:
        std::vector<DylibData> dylib_stack_{};
        uint32 reload_count_{};
    };
}
