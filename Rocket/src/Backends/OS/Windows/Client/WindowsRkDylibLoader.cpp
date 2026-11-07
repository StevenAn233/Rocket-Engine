module;

#include <windows.h>

module RkDylibLoader;

import Log;
import FileUtils;

namespace rke
{
// WindowsRkDylib
    struct WindowsRkDylib : public RkDylib
    {
    public:
        friend class RkDylibLoader;

        WindowsRkDylib(const Path& path);
        ~WindowsRkDylib() override;
    private:
        HMODULE handle_{};
    };

    WindowsRkDylib::WindowsRkDylib(const Path& path) : RkDylib()
    {
        using ScriptsGetter = void (*)(ScriptsView*);

        if(path.empty()) {
            CORE_ERROR(u8"WindowsRkDylib: Dylib path empty!");
            return;
        }
        handle_ = LoadLibraryW(path.get().c_str());
        if(!handle_) {
            CORE_ERROR(u8"WindowsRkDylib: "
                u8"Failed to load file '{}'! May be occupied.", path);
            return;
        }
    // emplace all script types & recipes
        void* scripts_getter{ GetProcAddress(handle_, "rke_fuel_all_scripts") };
        if(!scripts_getter) {
            CORE_ERROR(u8"WindowsRkDylib: "
                u8"Function 'rke_fuel_all_scripts' not found!");
            FreeLibrary(handle_); handle_ = {};
            return;
        }
        ScriptsView view{};
        std::invoke(std::bit_cast<ScriptsGetter>(scripts_getter), &view);
        for(uint64 i{}; i < view.size; i++)
        {
            const ScriptEntry& entry{ *(view.data + i) };
            script_types_.push_back(entry.type);
            script_recipes_.emplace(static_cast<uintptr>(entry.type), entry.recipe);
        }
    }

    WindowsRkDylib::~WindowsRkDylib()
    {
        script_types_.clear();
        script_recipes_.clear();
        if(handle_) FreeLibrary(handle_);
    }

// RkDylibLoader
    StringView RkDylibLoader::dylib_postfix() { return u8".dll"; }

    Scope<RkDylib> RkDylibLoader::load_fresh()
    {
        auto dylib{ create_scope<WindowsRkDylib>(copy_dylib_file()) };
        if(dylib && dylib->handle_)
        {
            prune_temp_files();
            reload_count_++;
            return dylib;
        }
        CORE_ERROR(u8"RkDylibLoader: Failed to load dylib!");
        return nullptr;
    }
}
