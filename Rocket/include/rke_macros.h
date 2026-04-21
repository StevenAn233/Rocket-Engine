#pragma once

#ifdef RKE_PLATFORM_WINDOWS
    #ifdef RKE_STATIC // not gonna use
        #define RKE_API
        #define RKE_SCRIPT_API
    #else
        #ifdef RKE_BUILD_DLL
            #define RKE_API __declspec(dllexport)
        #else
            #define RKE_API __declspec(dllimport)
        #endif

        #ifdef RKE_SCRIPT_BUILD_DLL
            #define RKE_SCRIPT_API __declspec(dllexport)
        #else
            #define RKE_SCRIPT_API __declspec(dllimport)
        #endif
    #endif
#else
    #define RKE_API
    #define RKE_SCRIPT_API
#endif

// the names MUST be consistent with which in
// ${root}/assets/proj-templates/CMakePresets.json.txt && ${root}/CMakePresets.json
#ifdef RKE_PLATFORM_WINDOWS
    #ifdef RKE_DEBUG
        #define RKE_CONFIG_NAME u8"windows-x64-debug"
    #elif defined(RKE_RELEASE)
        #define RKE_CONFIG_NAME u8"windows-x64-release"
    #elif defined(RKE_SHIPPING)
        #define RKE_CONFIG_NAME u8"windows-x64-shipping"
    #else
        #define RKE_CONFIG_NAME u8"windows-x64-undefined"
    #endif
#else
    #define RKE_CONFIG_NAME u8"undefined"
#endif

#ifdef RKE_ENABLE_ASSERTS
    #define DEBUG_BREAK __debugbreak()
#else
    #define DEBUG_BREAK
#endif

#ifdef RKE_ENABLE_LOG
    #define CORE_TRACE(...)	   ::rke::log_impl(::rke::LogType::Core, ::rke::LogLevel::Trace, \
                                 std::source_location::current(), __VA_ARGS__)
    #define CORE_INFO(...)	   ::rke::log_impl(::rke::LogType::Core, ::rke::LogLevel::Info, \
                                 std::source_location::current(), __VA_ARGS__)
    #define CORE_WARN(...)	   ::rke::log_impl(::rke::LogType::Core, ::rke::LogLevel::Warn, \
                                 std::source_location::current(), __VA_ARGS__)
    #define CORE_ERROR(...)	   ::rke::log_impl(::rke::LogType::Core, ::rke::LogLevel::Error, \
                                 std::source_location::current(), __VA_ARGS__)
    #define CORE_CRITICAL(...) ::rke::log_impl(::rke::LogType::Core, ::rke::LogLevel::Critical, \
                                 std::source_location::current(), __VA_ARGS__)

    #define RKE_TRACE(...)	  ::rke::log_impl(::rke::LogType::Client, ::rke::LogLevel::Trace, \
                                std::source_location::current(), __VA_ARGS__)
    #define RKE_INFO(...)	  ::rke::log_impl(::rke::LogType::Client, ::rke::LogLevel::Info, \
                                std::source_location::current(), __VA_ARGS__)
    #define RKE_WARN(...)	  ::rke::log_impl(::rke::LogType::Client, ::rke::LogLevel::Warn, \
                                std::source_location::current(), __VA_ARGS__)
    #define RKE_ERROR(...)	  ::rke::log_impl(::rke::LogType::Client, ::rke::LogLevel::Error, \
                                std::source_location::current(), __VA_ARGS__)
    #define RKE_CRITICAL(...) ::rke::log_impl(::rke::LogType::Client, ::rke::LogLevel::Critical, \
                                std::source_location::current(), __VA_ARGS__)

// Assert
    #define CORE_ASSERT(x, ...) { if(!(x)) { CORE_CRITICAL(__VA_ARGS__); DEBUG_BREAK; } }
    #define RKE_ASSERT(x, ...)  { if(!(x)) { RKE_CRITICAL (__VA_ARGS__); DEBUG_BREAK; } }

#else
    #define CORE_TRACE(...)
    #define CORE_INFO(...)
    #define CORE_WARN(...)
    #define CORE_ERROR(...)
    #define CORE_CRITICAL(...)

    #define RKE_TRACE(...)
    #define RKE_INFO(...)
    #define RKE_WARN(...)
    #define RKE_ERROR(...)
    #define RKE_CRITICAL(...)

    #define CORE_ASSERT(x, ...)
    #define RKE_ASSERT(x, ...)
#endif

#if RKE_ENABLE_PROFILE
    // Resolve which function signature macro will be used.
    // Note that this only is resolved when the (pre)compiler starts,
    // so the syntax highlighting could mark the wrong one in your editor!
    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
        #define RKE_FUNC_SIG u8"" __PRETTY_FUNCTION__
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
        #define RKE_FUNC_SIG u8"" __PRETTY_FUNCTION__
    #elif (defined(__FUNCSIG__) || (_MSC_VER))
        #define RKE_FUNC_SIG u8"" __FUNCSIG__
    #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
        #define RKE_FUNC_SIG u8"" __FUNCTION__
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
        #define RKE_FUNC_SIG u8"" __FUNC__
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
        #define RKE_FUNC_SIG u8"" __func__
    #elif defined(__cplusplus) && (__cplusplus >= 201103)
        #define RKE_FUNC_SIG u8"" __func__
    #else
        #define RKE_FUNC_SIG "RKE_FUNC_SIG unknown!"
    #endif

    #define RKE_PROFILE_BEGIN_SESSION(name, filepath) \
        ::rke::Instrumentor::get().begin_session(name, filepath)
    #define RKE_PROFILE_END_SESSION() ::rke::Instrumentor::get().end_session()

    #define RKE_PROFILE_SCOPE_LINE_MID(name, line) \
        constexpr auto fixed_name##line \
            { ::rke::InstrumentorUtils::cleanup_output_string(name, u8"__cdecl ") }; \
        ::rke::InstrumentationTimer timer##line(::rke::StringView(fixed_name##line.data))
    // attach **line** to "fixed_name" and "timer" to prevent redefining issues
    #define RKE_PROFILE_SCOPE_LINE(name, line) RKE_PROFILE_SCOPE_LINE_MID(name, line)
    // go through a _MID to make sure __LINE__ is correctly expanded to integer
    #define RKE_PROFILE_SCOPE(name) RKE_PROFILE_SCOPE_LINE(name, __LINE__)
    #define RKE_PROFILE_FUNCTION() RKE_PROFILE_SCOPE(RKE_FUNC_SIG)
#else
    #define RKE_PROFILE_BEGIN_SESSION(name, filepath)
    #define RKE_PROFILE_END_SESSION()
    #define RKE_PROFILE_SCOPE(name)
    #define RKE_PROFILE_FUNCTION()
#endif
