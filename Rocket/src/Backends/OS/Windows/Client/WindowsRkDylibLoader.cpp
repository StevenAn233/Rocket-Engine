module;

#include <windows.h>

module RkDylibLoader;
import :Windows;

import Log;
import Types;
import Script;
import ScriptRegistry;
import FileUtils;

namespace rke
{
    WindowsRkDylib::WindowsRkDylib(const Path& path) : RkDylib()
    {
        handle_ = LoadLibraryA(path.string().raw());
        if(!handle_) CORE_ERROR(u8"WindowsRkDylib: "
            u8"Failed to load .dll file '{}'! May be occupied.", path);
    }

    WindowsRkDylib::~WindowsRkDylib() { if(handle_) FreeLibrary(handle_); }

    bool WindowsRkDylib::is_valid() const { return (handle_ != nullptr); }

    ScriptsRegistar WindowsRkDylib::get_scripts_registar() const
    {
        if(!is_valid()) return nullptr;
        void* proc_addr{ GetProcAddress(handle_, "register_scripts") };
        if(!proc_addr) CORE_ERROR(u8"WindowsRkDylib: "
            u8"Function 'register_scripts' not found!");
        return std::bit_cast<ScriptsRegistar>(proc_addr);
    }

    WindowsRkDylibLoader::~WindowsRkDylibLoader() { delete_temp_files(); }

    Scope<RkDylib> WindowsRkDylibLoader::load_new()
    {
    // Hot-reloading Support
        Path dylib_path{ dylib_dir_ / String::format(u8"{}.dll", dylib_name_) };
        if(!dylib_path.exists())
        {
            CORE_ERROR(u8"WindowsRkDylibLoader: "
                u8"Dylib path '{}' doesn't exist!", dylib_path);
            return nullptr;
        }

        Path copy_dylib_path{ dylib_dir_ / String::format
            (u8"{}.loaded-{}.dll", dylib_name_, reload_count_++) };
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
            CORE_ERROR(u8"WindowsRkDylibLoader: "
                u8"Failed to copy DLL for hot-reloading!\n -- {}", e.what());
            return nullptr;
        }

        return create_scope<WindowsRkDylib>(copy_dylib_path);
    }

    void WindowsRkDylibLoader::delete_temp_files()
    {
        if(!fs::exists(dylib_dir_)) return;
        for(const auto& entry : fs::directory_iterator(dylib_dir_))
        {
            String filename{ Path(entry.path().filename()).string() };
            if(filename.find(u8"loaded") != String::npos)
            {
                std::error_code ec{};
                fs::remove(entry.path(), ec);
                if(!ec) CORE_INFO(u8"Project: Deleted temporary '{}'.", filename);
            }
        }
    }
}
