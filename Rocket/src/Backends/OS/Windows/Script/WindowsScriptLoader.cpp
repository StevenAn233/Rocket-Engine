module;

#include <windows.h>

module ScriptLoader;

import Types;
import Log;
import String;
import Script;
import ScriptRegistry;
import FileUtils;

namespace {
    struct HModuleDeleter{ void operator()(HMODULE h) const { if(h) FreeLibrary(h); }};
    using SCOPE_HMODULE = std::unique_ptr<std::remove_pointer_t<HMODULE>, HModuleDeleter>;

    static std::vector<SCOPE_HMODULE> s_dll_handles{};
    static rke::uint32 s_reload_count{};
}

namespace rke
{
    bool ScriptLoader::load_script_dll(const Path& dllpath)
    {
        if(!dllpath.exists()) {
            CORE_ERROR(u8"WindowsScriptLoader: DLL not found at '{}'!", dllpath);
            return false;
        }

    // Hot-reloading Support
        Path copy_dll_path{ dllpath };
        copy_dll_path.replace_extension(String::format(u8"loaded_{}.dll", s_reload_count++));

        Path pdb_path{ dllpath };
        pdb_path.replace_extension(u8".pdb"); // share the same name

        Path copy_pdb_path{ copy_dll_path };
        copy_pdb_path.replace_extension(u8"pdb");

        try {
            // copy DLL
            fs::copy_file(dllpath, copy_dll_path,
                fs::copy_options::overwrite_existing); // Important

            // copy PDB
            if(pdb_path.exists()) {
                fs::copy_file(pdb_path, copy_pdb_path,
                    fs::copy_options::overwrite_existing); // Important
            }
        }
        catch(const fs::filesystem_error& e)
        {
            CORE_ERROR(u8"WindowsScriptLoader: Failed to copy DLL for hot-reloading!\n -- {}", e.what());
            return false;
        }

        SCOPE_HMODULE handle{ LoadLibraryA(copy_dll_path.string().raw()) }; // pointer to dll
        if(!handle)
        {
            CORE_ERROR(u8"WindowsScriptLoader: Failed to load DLL '{}'!", dllpath);
            return false;
        }

        typedef void (*RegisterScriptsFunc)(); // pointer to register function
        RegisterScriptsFunc register_scripts
            { RegisterScriptsFunc(GetProcAddress(handle.get(), "register_scripts"))};
        // function name has to be exactly the same

        if(register_scripts)
        {
            CORE_INFO(u8"WindowsScriptLoader: Registering scripts from '{}'...", dllpath);
            ScriptRegistry::clear();
            register_scripts(); // call the register function
            CORE_INFO(u8"WindowsScriptLoader: Scripts Registered.");

            unload_all();
            s_dll_handles.push_back(std::move(handle));
            return true;
        }

        CORE_ERROR(u8"WindowsScriptLoader: Could not find 'register_scripts' in DLL!");
        return false;
    }

    void ScriptLoader::unload_all()
    {
        if(!s_dll_handles.empty())
        {
            s_dll_handles.clear();
            CORE_INFO(u8"WindowsScriptLoader: Unloaded all script DLLs.");
        }
    }
}
