module;
module RkDylibLoader;

import Log;

namespace rke
{
    const char8* RkDylibLoader::s_temp_rep_{ u8".loaded-" };

    RkDylibLoader::RkDylibLoader(Path dir, String name)
        : dylib_dir_(std::move(dir)), dylib_name_(std::move(name)) {}
    
    RkDylibLoader::~RkDylibLoader() { delete_temp_files(); }

    String RkDylibLoader::temp_name_of(uint32 index)
    {
        return String::format(u8"{}{}{}",
            dylib_name_, StringView(s_temp_rep_), index);
    }

    void RkDylibLoader::prune_temp_files()
    {
        if(!fs::exists(dylib_dir_)) return;

        // reload_count_ - 1 is the copy handed out by the last successful load
        const String keep_stem{ (reload_count_ == 0) ?
            String{} : temp_name_of(reload_count_ - 1) };

        for(const auto& entry : fs::directory_iterator(dylib_dir_))
        {
            const String filename{ Path(entry.path().filename()).string() };
            if(filename.find(StringView(s_temp_rep_)) == String::npos) continue;
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
            if(filename.find(StringView(s_temp_rep_)) != String::npos)
            {
                std::error_code ec{};
                fs::remove(entry.path(), ec);
                if(!ec) CORE_INFO(u8"RkDylibLoader: Deleted temporary '{}'.", filename);
            }
        }
    }
}
