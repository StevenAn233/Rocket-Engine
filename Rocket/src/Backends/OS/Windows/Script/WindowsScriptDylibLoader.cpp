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
    void HModuleDeleter::operator()(HMODULE h) const { if(h) FreeLibrary(h); }

    WindowsScriptDylibLoader::~WindowsScriptDylibLoader()
    {
        clear_cache();
        delete_temp_files();
    }

    bool WindowsScriptDylibLoader::load_dylib()
    {
    // Hot-reloading Support
        Path dll_path{ dylib_dir_ / String::format(u8"{}.dll", dylib_name_) };
        if(!dll_path.exists()) {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Dylib path '{}' doesn't exist!", dll_path);
            return false;
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
            return false;
        }

        DylibData dylib{ LoadLibraryA(copy_dll_path.string().raw()) };
        if(!dylib) {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Failed to load .dll file '{}'! May be occupied.", dylib_dir_);
            return false;
        }

        func_ = reinterpret_cast<RegisterScriptsFunc>(GetProcAddress(dylib.get(), "register_scripts"));
        // function name has to be exactly the same(ScriptRegistry)
        if(func_) {
            dylib_stack_.push_back(std::move(dylib));
            return true;
        }
        CORE_ERROR(u8"WindowsScriptDylibLoader: Could not find 'register_scripts' in .dll!");
        return false;
    }

    void WindowsScriptDylibLoader::clear_cache()
    {
        if(!dylib_stack_.empty())
        {
            dylib_stack_.clear();
            CORE_INFO(u8"WindowsScriptDylibLoader: Unloaded all script DLLs.");
        }
    }

    void WindowsScriptDylibLoader::delete_temp_files()
    {
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
