module;
module Log;

import Application;

namespace {
    using namespace rke;

    namespace color_sv
    {
        constexpr StringView reset   { u8"\033[0m" };
        constexpr StringView red     { u8"\033[0;1;31m"	};
        constexpr StringView green   { u8"\033[0;1;32m"	};
        constexpr StringView yellow  { u8"\033[0;1;33m"	};
        constexpr StringView white   { u8"\033[0;1;37m"	};
        constexpr StringView grey    { u8"\033[0;2;37m"	};
        constexpr StringView critical{ u8"\033[0;1;37;41m" };
    }

    constexpr StringView get_color_sv(LogLevel level)
    {
        switch(level)
        {
            case LogLevel::Info:     return color_sv::green;
            case LogLevel::Warn:     return color_sv::yellow;
            case LogLevel::Error:    return color_sv::red;
            case LogLevel::Trace:    return color_sv::grey;
            case LogLevel::Critical: return color_sv::critical;
            default:                 return color_sv::reset;
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
    void log(LogType type, LogLevel level,
        const std::source_location& loc, const char8* msg)
    {
        auto now{ std::chrono::system_clock::now() };
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), now) };

        LogDest dest{ LogDest::Standard };
        if(!app_null()) dest = app().log_dest();

        switch(dest)
        {
        case LogDest::Editor:
            if(!app_null())
            {
                String output{ msg };
                app().push_text_log(std::move(output));
            }
            break;
        case LogDest::Standard:
        default:
            // Format: 'Color' [Time][Prefix] [Filename(line, col)] Msg 'Reset'
            std::println("{}[{:%T}][{}] [File: {}({}, {})]\n    {}{}{}",
                color_sv::grey, zoned_time, get_prefix(type),
                str::extract_filename(StringView(str::to_char8(loc.file_name()))),
                loc.line(), loc.column(), get_color_sv(level), String(msg), color_sv::reset);
            break;
        }
    }

    void log(LogType type, LogLevel level,
        const std::source_location& loc, const Event& e)
    {
        auto now{ std::chrono::system_clock::now() };
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), now) };

        LogDest dest{ LogDest::Standard };
        if(!app_null()) dest = app().log_dest();

        switch(dest)
        {
        case LogDest::Editor:
            if(!app_null())
            {
                String output{ e.to_string() };
                app().push_text_log(std::move(output));
            }
            break;
        case LogDest::Standard:
        default:
            // Format: 'Color' [Time][Prefix] [Window-Title] [Filename(line, col)] e.msg 'Reset'
            std::println("{}[{:%T}][{}] [File: {}({}, {})]\n    {}(Window: {}) {}{}",
                color_sv::grey, zoned_time, get_prefix(type),
                str::extract_filename(StringView(str::to_char8(loc.file_name()))),
                loc.line(), loc.column(), get_color_sv(level),
                e.get_window_name(), e.to_string(), color_sv::reset);
            break;
        }
    }
}
