module;

#include <format>
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
    enum class LogDest { Standard, Editor };

    RKE_API void log(LogType type, LogLevel level,
        const std::source_location& loc, const char8* msg);

    template<typename... Args>
    void log(LogType type, LogLevel level, const std::source_location& loc,
        U8FormatString<std::type_identity_t<Args>...> fmt, Args&&... args)
    {
        String msg{ String::format(fmt, std::forward<Args>(args)...) };
        log(type, level, loc, msg.c_str());
    }

    RKE_API void log(LogType Type, LogLevel Level,
        const std::source_location& loc, const Event& e);
}
