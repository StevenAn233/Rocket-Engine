module;

#include <chrono>
#include <print>

module Log;

namespace {
    using namespace rke;

    namespace color
    {
        constexpr StringView reset   { u8"\033[0m" };
        constexpr StringView red     { u8"\033[0;1;31m"	};
        constexpr StringView green   { u8"\033[0;1;32m"	};
        constexpr StringView yellow  { u8"\033[0;1;33m"	};
        constexpr StringView white   { u8"\033[0;1;37m"	};
        constexpr StringView grey    { u8"\033[0;2;37m"	};
        constexpr StringView critical{ u8"\033[0;1;37;41m" };
    }

    constexpr StringView get_color(LogLevel level)
    {
        switch(level)
        {
            case LogLevel::Info:     return color::green;
            case LogLevel::Warn:     return color::yellow;
            case LogLevel::Error:    return color::red;
            case LogLevel::Trace:    return color::grey;
            case LogLevel::Critical: return color::critical;
            default:                 return color::reset;
        };
    }

    constexpr StringView get_prefix(LogType type)
    {
        using namespace rke::literals;
        return (type == LogType::Core) ? u8"ROCKET"_sv : u8"CLIENT"_sv;
    }
}

namespace rke
{
    void log_impl(LogType type, LogLevel level,
        const std::source_location& loc, const char8* msg)
    {
        auto now{ std::chrono::system_clock::now() };
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), now) };

        // Format: 'Color' [Time][Prefix] [Filename(line, col)] Msg 'Reset'
        std::println("{}[{:%T}][{}] [File: {}({}, {})]\n    {}{}{}",
            color::grey, zoned_time, get_prefix(type),
            str::extract_filename(StringView(str::to_char8(loc.file_name()))),
            loc.line(), loc.column(), get_color(level), String(msg), color::reset);
    }

    void log_impl(LogType type, LogLevel level,
        const std::source_location& loc, const Event& e)
    {
        auto now{ std::chrono::system_clock::now() };
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), now) };

        // Format: 'Color' [Time][Prefix] [Window-Title] [Filename(line, col)] e.msg 'Reset'
        std::println("{}[{:%T}][{}] [File: {}({}, {})]\n    {}(Window: {}) {}{}",
            color::grey, zoned_time, get_prefix(type),
            str::extract_filename(StringView(str::to_char8(loc.file_name()))),
            loc.line(), loc.column(), get_color(level),
            e.get_window_title(), e.to_string(), color::reset);
    }
}
