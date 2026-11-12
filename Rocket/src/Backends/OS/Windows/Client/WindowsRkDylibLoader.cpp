module;

#include <windows.h>

module RkDylibLoader;

import Log;
import FileUtils;
import ScriptAccess;
import EffectAccess;

namespace rke
{
// WindowsRkDylib
    class WindowsRkDylib : public RkDylib
    {
    public:
        friend class RkDylibLoader;

        WindowsRkDylib(const Path& path);
        ~WindowsRkDylib() override;
    private:
        HMODULE handle_{};
    };

    WindowsRkDylib::WindowsRkDylib(const Path& path)
    {
        using ScriptsGetter = void (*)(ScriptsView*);
        using EffectsGetter = void (*)(EffectsView*);

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

        void* scripts_getter{ GetProcAddress(handle_, "rke_fuel_all_scripts") };
        if(!scripts_getter) {
            CORE_ERROR(u8"WindowsRkDylib: "
                u8"Function 'rke_fuel_all_scripts' not found!");
            FreeLibrary(handle_); handle_ = {};
            return;
        }
        void* effects_getter{ GetProcAddress(handle_, "rke_fuel_all_effects") };
        if(!effects_getter) {
            CORE_ERROR(u8"WindowsRkDylib: "
                u8"Function 'rke_fuel_all_effects' not found!");
            FreeLibrary(handle_); handle_ = {};
            return;
        }

    // emplace all script types & recipes
        ScriptsView scripts_view{};
        std::invoke(std::bit_cast<ScriptsGetter>(scripts_getter), &scripts_view);
        for(uint64 i{}; i < scripts_view.size; i++)
        {
            const ScriptEntry& entry{ *(scripts_view.data + i) };
            script_types_.push_back(entry.type);
            script_recipes_.emplace(static_cast<uintptr>(entry.type), entry.recipe);
        }
    // emplace all effect types & recipes
        EffectsView effects_view{};
        std::invoke(std::bit_cast<EffectsGetter>(effects_getter), &effects_view);
        for(uint64 i{}; i < effects_view.size; i++)
        {
            const EffectEntry& entry{ *(effects_view.data + i) };
            effect_types_.push_back(entry.type);
            effect_recipes_.emplace(static_cast<uintptr>(entry.type), entry.constructor);
        }
    }

    WindowsRkDylib::~WindowsRkDylib()
    {
        script_types_.clear();
        script_recipes_.clear();
        effect_types_.clear();
        effect_recipes_.clear();
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
