module;

// required
#include <format>
#include <filesystem>

#include <utility>
#include "rke_macros.h"

export module Path;

import String;
import Types;

export namespace rke
{
    namespace fs = std::filesystem;

    class RKE_API Path final
    {
    public:
        Path() = default;
        Path(const Path& p) noexcept : handle_(p) {}
        Path(Path&& p) noexcept : handle_(std::move(p)) {}
        Path(fs::path p) : handle_(std::move(p)) {}
        Path(const String& s) : handle_(s.c_str()) {}
        Path(StringView sv) : handle_(sv.data(), sv.data() + sv.size()) {}
        template<Size N>
        Path(const char8 (&s)[N]) : handle_(s) {}
        explicit Path(const char8* s) : handle_(s) {}

        constexpr Path& operator=(const Path& other) noexcept
        {
            handle_ = other.handle_;
            return *this;
        }
        constexpr Path& operator=(Path&& other) noexcept
        {
            handle_ = std::move(other.handle_);
            return *this;
        }

        String string() const { return String(handle_.u8string()); }

        // for io/file reading
        operator const fs::path&() const { return handle_; }
        const fs::path& get() const { return handle_; }

        bool empty() const { return handle_.empty(); }
        Path stem () const { return Path(handle_.stem()); }
        Path extension  () const { return Path(handle_.extension  ()); }
        Path filename   () const { return Path(handle_.filename   ()); }
        Path parent_path() const { return Path(handle_.parent_path()); }
        template<Size N>
        Path& replace_extension(const char8 (&s)[N])
        {
            handle_.replace_extension(fs::path(s));
            return *this;
        }
        Path& replace_extension(const String& s)
        {
            handle_.replace_extension(s.get());
            return *this;
        }
        void clear() noexcept { handle_.clear(); }

        Path operator/(const Path& other) const
            { return Path(handle_ / other.handle_); }
        Path& operator/=(const Path& other)
            { handle_ /= other.handle_; return *this;  }

        bool operator==(const Path& other) const { return handle_ == other.handle_; }

        bool exists() const { return fs::exists(handle_); }
    private:
        fs::path handle_{};
    }; 
}

namespace std
{
    export template<>
    struct formatter<rke::Path> : formatter<string_view>
    {
        auto format(const rke::Path& p, format_context& ctx) const
        {
            auto u8str{ static_cast<const filesystem::path&>(p).u8string() };
            string_view sv(reinterpret_cast<const char*>(u8str.data()), u8str.size());
            return formatter<string_view>::format(sv, ctx);
        }
    };
}
