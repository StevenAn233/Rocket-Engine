module;

#include <windows.h>

module ScriptLoader;

import Log;
import Types;
import Script;
import ScriptRegistry;
import FileUtils;

namespace
{
    struct HModuleDeleter{ void operator()(HMODULE h) const { if(h) FreeLibrary(h); }};
    using SCOPE_HMODULE = std::unique_ptr<std::remove_pointer_t<HMODULE>, HModuleDeleter>;

    static std::vector<SCOPE_HMODULE> s_dll_handles{};
    static rke::uint32 s_reload_count{};
}

namespace rke
{
    bool ScriptLoader::load_dylib(const Path& dir, const String& name)
    {
        ScriptRegistry::clear();

        if(!dir.exists()) {
            CORE_ERROR(u8"WindowsScriptLoader: "
                u8"Directory '{}' doesn't exist!", dir);
            return false;
        }

    // Hot-reloading Support
        Path dll_path{ dir / String::format(u8"{}.dll", name) };
        Path copy_dll_path{ dir / String::format(u8"{}.loaded-{}.dll", name, s_reload_count++) };
        CORE_ASSERT(dll_path.exists(), u8"WindowsScriptLoader: "
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
            CORE_ERROR(u8"WindowsScriptLoader: "
                u8"Failed to copy DLL for hot-reloading!\n -- {}", e.what());
            return false;
        }

        SCOPE_HMODULE dll_handle{ LoadLibraryA(copy_dll_path.string().raw()) };
        if(!dll_handle) {
            CORE_ERROR(u8"WindowsScriptLoader: "
                u8"Failed to load DLL '{}'! May be occupied.", dir);
            return false;
        }

        auto register_scripts{ reinterpret_cast<void(*)()>
            (GetProcAddress(dll_handle.get(), "register_scripts")) };
        // function name has to be exactly the same(ScriptRegistry)
        if(register_scripts)
        {
            CORE_INFO(u8"WindowsScriptLoader: Registering scripts from '{}'...", dir);
            register_scripts(); // call the register function

            s_dll_handles.clear();
            s_dll_handles.push_back(std::move(dll_handle));

            CORE_INFO(u8"WindowsScriptLoader: Scripts Registered.");
            return true;
        }

        CORE_ERROR(u8"WindowsScriptLoader: Could not find 'register_scripts' in DLL!");
        return false;
    }

    void ScriptLoader::unload_all_dylibs()
    {
        ScriptRegistry::clear();
        if(!s_dll_handles.empty())
        {
            s_dll_handles.clear();
            CORE_INFO(u8"WindowsScriptLoader: Unloaded all script DLLs.");
        }
    }

    void ScriptLoader::delete_temp_files(const Path& dir)
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
