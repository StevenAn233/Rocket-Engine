module;

#include <chrono>
#include <print>
#include <vector>
#include <utility>

module Log;

import String;
import Types;
import Event;
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

    static void console_out(const LogEntry& entry)
    {
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), entry.time) };
        String file_name{ str::extract_filename(StringView(str::to_char8(entry.file))) };

        if(entry.window_name.empty()) {
            // Format: 'Color' [Time][Prefix] [Filename(line, col)] Msg 'Reset'
            std::println("{}[{:%T}][{}] [File: {}({}, {})]\n    {}{}{}",
                color_sv::grey, zoned_time, get_prefix(entry.type), file_name,
                entry.line, entry.column, get_color_sv(entry.level),
                entry.message, color_sv::reset);
        } else {
            // Format: 'Color' [Time][Prefix] [Window-Title] [Filename(line, col)] e.msg 'Reset'
            std::println("{}[{:%T}][{}] [File: {}({}, {})]\n    {}(Window: {}) {}{}",
                color_sv::grey, zoned_time, get_prefix(entry.type), file_name,
                entry.line, entry.column, get_color_sv(entry.level),
                entry.window_name, entry.message, color_sv::reset);
        }
    }
}

namespace rke
{
    LogHistory log_history{};

    void LogHistory::push(LogEntry entry)
    {
        std::lock_guard lock{ mutex_ };
        entries_.push_back(std::move(entry));
        while(entries_.size() > capacity_)
        {
            entries_.pop_front();
            dropped_.fetch_add(1, std::memory_order_relaxed);
        }
        written_.fetch_add(1, std::memory_order_relaxed);
    }

    void LogHistory::clear()
    {
        std::lock_guard lock{ mutex_ };
        entries_.clear();
        dropped_.store(written_.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    uint64 LogHistory::copy_since(uint64 from, std::vector<LogEntry>& out) const
    {
        std::lock_guard lock{ mutex_ };
        const uint64 end  { written_.load(std::memory_order_relaxed) };
        const uint64 first{ dropped_.load(std::memory_order_relaxed) };
        if(from < first) from = first; // whatever was recycled is gone

        for(uint64 seq{ from }; seq < end; ++seq)
            out.push_back(entries_[seq - first]);
        return end;
    }

    void log(LogType type, LogLevel level,
        const std::source_location& loc, String str)
    {
        LogEntry entry
        {
            .type    = type,
            .level   = level,
            .time    = std::chrono::system_clock::now(),
            .file    = loc.file_name(),
            .line    = static_cast<uint32>(loc.line()),
            .column  = static_cast<uint32>(loc.column()),
            .message = std::move(str)
        };

        if(entry.level == LogLevel::Critical) console_out(entry); // may modify
        log_history.push(std::move(entry));
    }

    void log(LogType type, LogLevel level,
        const std::source_location& loc, const Event& e)
    {
        LogEntry entry
        {
            .type        = type,
            .level       = level,
            .time        = std::chrono::system_clock::now(),
            .file        = loc.file_name(),
            .line        = static_cast<uint32>(loc.line()),
            .column      = static_cast<uint32>(loc.column()),
            .message     = String(e.to_string()),
            .window_name = String(e.get_window_name())
        };

        if(entry.level == LogLevel::Critical) console_out(entry); // may modify
        log_history.push(std::move(entry));
    }
}
