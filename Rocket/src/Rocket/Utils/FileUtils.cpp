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

    Path root_dir()
    {
        Path dir{ executable_dir() / u8".." / u8".." };
        CORE_ASSERT(dir.exists() && fs::is_directory(dir),
            u8"FileUtils: Root dir doesn't exist!");
        return fs::canonical(dir);
    }

    Path assets_dir()
    {
        Path assets_dir{ root_dir() / u8"assets" };
        if(assets_dir.exists() && fs::is_directory(assets_dir))
            return fs::canonical(assets_dir); // absolute
        CORE_ERROR(u8"FileUtils: Assets dir not found!");
        return {};
    }

    Path editor_dir()
    {
        Path editor_dir{ root_dir() / u8"editor" };
        if(editor_dir.exists() && fs::is_directory(editor_dir))
            return fs::canonical(editor_dir); // absolute
        CORE_ERROR(u8"FileUtils: Editor dir not found!");
        return {};
    }

    Path shader_cache_dir()
    {
        Path build_dir{ root_dir() / u8"bin" };
        if(!build_dir.exists()) fs::create_directory(build_dir);
        Path shader_cache_dir{ build_dir / u8"shaders" };
        if(!shader_cache_dir.exists()) fs::create_directory(shader_cache_dir);
        return shader_cache_dir;
    }
}
