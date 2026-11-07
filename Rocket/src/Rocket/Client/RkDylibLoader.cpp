module;
module RkDylibLoader;

import Log;

namespace {
    using namespace rke;
    StringView temp_represent() { return u8".loaded-"; }

    String temp_name_of(const String& dylib_name, uint32 index)
    {
        return String::format(u8"{}{}{}",
            dylib_name, temp_represent(), index);
    }
}

namespace rke
{
    RkDylibLoader::RkDylibLoader(Path dir, String name)
        : dylib_dir_(std::move(dir)), dylib_name_(std::move(name)) {}
    
    RkDylibLoader::~RkDylibLoader() { delete_temp_files(); }

    Path RkDylibLoader::copy_dylib_file()
    {
        Path dylib_path{ dylib_dir_ / (dylib_name_ + dylib_postfix()) };
        if(!dylib_path.exists())
        {
            CORE_ERROR(u8"RkDylibLoader: "
                u8"Dylib path '{}' doesn't exist!", dylib_path);
            return {};
        }
        
        Path copy_dylib_path{ dylib_dir_ /
            (temp_name_of(dylib_name_, reload_count_) + dylib_postfix()) };
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
                u8"Failed to copy dylib for hot-reloading!\n - {}", e.what());
            return {};
        }
        return copy_dylib_path;
    }

    void RkDylibLoader::prune_temp_files()
    {
        if(!fs::exists(dylib_dir_)) return;

        // reload_count_ - 1 is the copy handed out by the last successful load
        const String keep_stem{ (reload_count_ == 0) ?
            String{} : temp_name_of(dylib_name_, reload_count_ - 1) };

        for(const auto& entry : fs::directory_iterator(dylib_dir_))
        {
            const String filename{ Path(entry.path().filename()).string() };
            if(filename.find(temp_represent()) == String::npos) continue;
            // matches both the .dll and its .pdb
            if(Path(filename).get().stem().u8string() == keep_stem.get()) continue;

            std::error_code ec{};
            fs::remove(entry.path(), ec);
            if(!ec) CORE_INFO(u8"RkDylibLoader: Deleted stale '{}'.", filename);
        }
    }

    void RkDylibLoader::delete_temp_files()
    {
        if(!fs::exists(dylib_dir_)) return;
        for(const auto& entry : fs::directory_iterator(dylib_dir_))
        {
            String filename{ Path(entry.path().filename()).string() };
            if(filename.find(temp_represent()) != String::npos)
            {
                std::error_code ec{};
                fs::remove(entry.path(), ec);
                if(!ec) CORE_INFO(u8"RkDylibLoader: Deleted temporary '{}'.", filename);
            }
        }
    }
}
