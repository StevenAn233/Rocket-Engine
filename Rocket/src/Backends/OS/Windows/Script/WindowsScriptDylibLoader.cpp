module;

#include <windows.h>

module ScriptDylibLoader;
import :Windows;

import Log;
import Types;
import Script;
import ScriptRegistry;
import FileUtils;

namespace rke
{
    WindowsDylib::WindowsDylib(const Path& path) : ScriptDylib()
    {
        handle_ = LoadLibraryA(path.string().raw());
        if(!handle_) CORE_ERROR(u8"WindowsDylib: "
            u8"Failed to load .dll file '{}'! May be occupied.", path);
    }

    WindowsDylib::~WindowsDylib() { if(handle_) FreeLibrary(handle_); }

    bool WindowsDylib::valid() const { return (handle_ != nullptr); }

    ScriptsRegistar WindowsDylib::get_scripts_registar() const
    {
        if(!valid()) return nullptr;
        void* proc_addr{ GetProcAddress(handle_, "register_scripts") };
        if(!proc_addr) CORE_ERROR(u8"WindowsDylib: "
            u8"Function 'register_scripts' not found!");
        return std::bit_cast<ScriptsRegistar>(proc_addr);
    }

    WindowsScriptDylibLoader::~WindowsScriptDylibLoader() { delete_temp_files(); }

    Scope<ScriptDylib> WindowsScriptDylibLoader::load_dylib() const
    {
    // Hot-reloading Support
        Path dll_path{ dylib_dir_ / String::format(u8"{}.dll", dylib_name_) };
        if(!dll_path.exists())
        {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Dylib path '{}' doesn't exist!", dll_path);
            return nullptr;
        }

        Path copy_dll_path{ dylib_dir_ / String::format
            (u8"{}.loaded-{}.dll", dylib_name_, reload_count_++) };
        try {
            // copy DLL
            fs::copy_file(dll_path, copy_dll_path,
                fs::copy_options::overwrite_existing); // Important

            // copy PDB
            Path pdb_path{ Path(dll_path).replace_extension(u8"pdb") };
            if(pdb_path.exists()) {
                Path copy_pdb_path{ Path(copy_dll_path).replace_extension(u8"pdb") };
                fs::copy_file(pdb_path, copy_pdb_path,
                    fs::copy_options::overwrite_existing); // Important
            }
        } catch(const fs::filesystem_error& e) {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Failed to copy DLL for hot-reloading!\n -- {}", e.what());
            return nullptr;
        }

        return create_scope<WindowsDylib>(copy_dll_path);
    }

    void WindowsScriptDylibLoader::delete_temp_files()
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
