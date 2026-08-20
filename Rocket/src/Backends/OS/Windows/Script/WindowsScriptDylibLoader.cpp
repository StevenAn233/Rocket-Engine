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
    using RegisterScriptsFunc = void(*)();

    void HModuleDeleter::operator()(HMODULE h) const { if(h) FreeLibrary(h); }

    bool WindowsScriptDylibLoader::load_dylib(const Path& dir, const String& name)
    {
        ScriptRegistry::clear();

        if(!dir.exists()) {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Directory '{}' doesn't exist!", dir);
            return false;
        }

    // Hot-reloading Support
        Path dll_path{ dir / String::format(u8"{}.dll", name) };
        Path copy_dll_path{ dir / String::format(u8"{}.loaded-{}.dll", name, reload_count_++) };
        CORE_ASSERT(dll_path.exists(), u8"WindowsScriptDylibLoader: "
            u8"Dll path '{}' doesn't exist!", dll_path);

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
        }
        catch(const fs::filesystem_error& e)
        {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Failed to copy DLL for hot-reloading!\n -- {}", e.what());
            return false;
        }

        DylibData dylib{ LoadLibraryA(copy_dll_path.string().raw()) };
        if(!dylib) {
            CORE_ERROR(u8"WindowsScriptDylibLoader: "
                u8"Failed to load .dll file '{}'! May be occupied.", dir);
            return false;
        }

        RegisterScriptsFunc register_scripts{ reinterpret_cast<RegisterScriptsFunc>
            (GetProcAddress(dylib.get(), "register_scripts")) };
        // function name has to be exactly the same(ScriptRegistry)
        if(register_scripts)
        {
            CORE_INFO(u8"WindowsScriptDylibLoader: Registering scripts from '{}'...", dir);
            register_scripts(); // call the register function

            dylib_stack_.clear();
            dylib_stack_.push_back(std::move(dylib));

            CORE_INFO(u8"WindowsScriptDylibLoader: Scripts Registered.");
            return true;
        }

        CORE_ERROR(u8"WindowsScriptDylibLoader: Could not find 'register_scripts' in DLL!");
        return false;
    }

    void WindowsScriptDylibLoader::unload_all_dylibs()
    {
        ScriptRegistry::clear();
        if(!dylib_stack_.empty())
        {
            dylib_stack_.clear();
            CORE_INFO(u8"WindowsScriptDylibLoader: Unloaded all script DLLs.");
        }
    }

    void WindowsScriptDylibLoader::delete_temp_files(const Path& dir)
    {
        if(!dir.exists()) return;
        for(const auto& entry : fs::directory_iterator(dir))
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
