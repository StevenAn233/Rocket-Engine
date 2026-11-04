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

        handle_ = LoadLibraryW(path.get().c_str());
        if(!handle_) {
            CORE_ERROR(u8"WindowsRkDylib: "
                u8"Failed to load .dll file '{}'! May be occupied.", path);
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
    Scope<RkDylib> RkDylibLoader::load_fresh()
    {
    // Hot-reloading Support
        Path dylib_path{ dylib_dir_ / (dylib_name_ + u8".dll") };
        if(!dylib_path.exists())
        {
            CORE_ERROR(u8"RkDylibLoader: "
                u8"Dylib path '{}' doesn't exist!", dylib_path);
            return nullptr;
        }
        
        Path copy_dylib_path{ dylib_dir_ / (temp_name_of(reload_count_) + u8".dll") };
        try {
            // copy DLL
            fs::copy_file(dylib_path, copy_dylib_path,
                fs::copy_options::overwrite_existing); // Important

            // copy PDB
            Path pdb_path{ Path(dylib_path).replace_extension(u8"pdb") };
            if(pdb_path.exists()) {
                Path copy_pdb_path{ Path(copy_dylib_path).replace_extension(u8"pdb") };
                fs::copy_file(pdb_path, copy_pdb_path,
                    fs::copy_options::overwrite_existing); // Important
            }
        } catch(const fs::filesystem_error& e) {
            CORE_ERROR(u8"RkDylibLoader: "
                u8"Failed to copy DLL for hot-reloading!\n - {}", e.what());
            return nullptr;
        }

        auto dylib{ create_scope<WindowsRkDylib>(copy_dylib_path) };
        if(dylib && dylib->handle_)
        {
            prune_temp_files();
            ++reload_count_;
            return dylib;
        }
        CORE_ERROR(u8"RkDylibLoader: Failed to load dylib!");
        return nullptr;
    }
}
