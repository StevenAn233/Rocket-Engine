module;
module FileUtils;

import Log;

namespace rke::file
{
    Path unify_path(String generic)
    {
        std::replace(generic.begin(), generic.end(), u8'/', u8'\\');
        return Path(generic);
    }

    Path find_assets_dir()
    {
        auto exe_dir{ get_executable_dir() };
        auto assets_dir{ exe_dir / u8".." / u8".." / u8"assets" };
        if(assets_dir.exists() && fs::is_directory(assets_dir))
            return fs::canonical(assets_dir); // absolute
        CORE_ERROR(u8"WindowsFileUtils: Assets dir not found!");
        return {};
    }

    Path find_editor_dir()
    {
        auto exe_dir{ get_executable_dir() };
        auto editor_dir{ exe_dir / u8".." / u8".." / u8"editor"};
        if(editor_dir.exists() && fs::is_directory(editor_dir))
            return fs::canonical(editor_dir); // absolute
        CORE_ERROR(u8"WindowsFileUtils: Editor dir not found!");
        return {};
    }

    Path get_shader_cache_dir()
    {
        Path build_dir{ find_assets_dir().parent_path() / u8".build" };
        if(!build_dir.exists()) fs::create_directory(build_dir);
        Path shader_cache_dir{ build_dir / u8"shaders" };
        if(!shader_cache_dir.exists()) fs::create_directory(shader_cache_dir);
        return shader_cache_dir;
    }
}
