module;

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <vector>
#include <utility>
#include <source_location>
#include "rke_macros.h"

export module Log;

import String;
import Types;
import Event;

export namespace rke 
{
    enum class LogType { Core, Client };
    enum class LogLevel{ Info, Warn, Error, Trace, Critical };

    struct RKE_API LogEntry
    {
        LogType  type { LogType ::Core };
        LogLevel level{ LogLevel::Info };
        std::chrono::system_clock::time_point time{};
        const char* file{ nullptr };
        uint32 line{ 0 };
        uint32 column{ 0 };
        String message{};
        String window_name{}; // events only
    };

    class RKE_API LogHistory
    {
    public:
        LogHistory() = default;
        ~LogHistory() = default;

        LogHistory(const LogHistory&) = delete;
        LogHistory& operator=(const LogHistory&) = delete;
        LogHistory(LogHistory&&) = delete;
        LogHistory& operator=(LogHistory&&) = delete;

        void push(LogEntry entry);
        void clear();

    // total number of entries ever pushed(monotonic sequence number)
        inline uint64 written() const { return written_.load(std::memory_order_relaxed); }
    // entries dropped because the ring buffer overflowed
        inline uint64 dropped() const { return dropped_.load(std::memory_order_relaxed); }

    // appends every kept entry whose sequence is >= from, returns the sequence
    // to hand in as 'from' next time
        uint64 copy_since(uint64 from, std::vector<LogEntry>& out) const;
    private:
        mutable std::mutex mutex_{};
        std::deque<LogEntry> entries_{};
        std::atomic<uint64> written_{ 0 };
        std::atomic<uint64> dropped_{ 0 };
        Size capacity_{ 8192 };
    };

    RKE_API void log(LogType type, LogLevel level,
        const std::source_location& loc, String str);

    RKE_API void log(LogType Type, LogLevel Level,
        const std::source_location& loc, const Event& e);

    inline void log(LogType type, LogLevel level,
        const std::source_location& loc, const char8* c_str)
        { log(type, level, loc, String(c_str)); }

    template<typename... Args>
    inline void log(LogType type, LogLevel level, const std::source_location& loc,
        U8FormatString<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        String msg{ String::format(fmt, std::forward<Args>(args)...) };
        log(type, level, loc, std::move(msg));
    }
}
