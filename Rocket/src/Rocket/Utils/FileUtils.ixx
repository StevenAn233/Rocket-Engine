module;

#include <string_view>
#include <optional>
#include <vector>
#include <filesystem>
#include "rke_macros.h"

export module FileUtils;

import Types;
import String;
import Path;
import Window;

export namespace rke
{
    class RKE_API FileDialogs
    {
    public:
    // windows api
        static std::optional<String> open_file(StringView filter, const Window* window);
        static std::optional<String> save_file(StringView filter, const Window* window);
        static std::optional<String> select_folder(const Window* window);
    };

    using Buffer = std::vector<rke::byte>;

    namespace file
    {
        inline void check_to_create_dir(const Path& path)
        {
            Path parent{ path.parent_path() };
            if(!parent.exists()) fs::create_directories(parent);
        }

    // Platform related
        RKE_API Buffer read_file_impl(const Path& file_path);

        inline Buffer read_file_binary(const Path& file_path) { return read_file_impl(file_path); }
        inline String read_file_string(const Path& file_path)
        {
            Buffer buffer{ read_file_impl(file_path)};
            return String(reinterpret_cast<const char8*>(buffer.data()), buffer.size());
        }

        RKE_API bool write_file_impl(const Path& path, const char* content, Size size, bool with_endl);

        template<typename Str>
        requires std::is_convertible_v<const Str&, std::string_view>
        bool write_file_string(const Path& path, const Str& s)
        {
            std::string_view sv{ s };
            return write_file_impl(path, sv.data(), sv.size(), true);
        }
        inline bool write_file_string(const Path& path, const char8* content)
            { return write_file_string(path, reinterpret_cast<const char*>(content)); }
        inline bool write_file_string(const Path& path, const String& content)
            { return write_file_string(path, content.raw()); }

        inline bool write_file_binary(const Path& path, const void* content, Size size)
            { return write_file_impl(path, reinterpret_cast<const char*>(content), size, false); }

        RKE_API Path unify_path(String generic);

        RKE_API Path get_executable_dir();
        RKE_API Path find_assets_dir();
        RKE_API Path find_editor_dir();
        RKE_API Path get_shader_cache_dir();
    }
}
