module;

#include <chrono>
#include <thread>
#include <mutex>
#include <fstream>
#include "rke_macros.h"

export module Instrumentor;
// drag the generated json file into "chrome://tracing/"

import Types;
import String;
import Path;

export namespace rke
{
    class RKE_API Instrumentor // Used for putting the result into a json file
    {
    public:
        struct ProfileResult
        {
            StringView name;

            std::chrono::duration<double, std::micro> start;
            std::chrono::microseconds elapsed_time;
            std::thread::id thread_id;
        };
        struct InstrumentationSession { String name; };

        Instrumentor(const Instrumentor&) = delete;
        Instrumentor(Instrumentor&& ____) = delete;

        // start a session on performance(lasting time)
        void begin_session(const String& name, const Path& filepath);
        // end the session
        void end_session();

        bool is_session_running() const { return is_session_active_; }
        float get_session_duration_s() const;

        // write the info(ProfileResult) into a json file during the session
        void write_profile(const ProfileResult& result);

        static Instrumentor& get()
        {
            static Instrumentor instance{}; // singleton
            return instance;
        }
    private:
        Instrumentor ();
        ~Instrumentor();

        // for write_profile()
        void write_header() { fout_ << "{\"otherData\": {},\"traceEvents\":[{}"; fout_.flush(); }
        void write_footer() { fout_ << "]}"; fout_.flush(); }

        // Note: you must already own lock on mutex_ before
        // calling internal_end_session()
        void internal_end_session();
    private:
        // use multy-threads to "create" sessions simultaneously(this class is a singleton)
        std::mutex mutex_;
        InstrumentationSession* current_session_{ nullptr };
        std::ofstream fout_;

        bool is_session_active_{ false };
        // might not support multi-thread
        std::chrono::steady_clock::time_point session_start_time_{};
    };

    class RKE_API InstrumentationTimer // Used to provide time datas we need
    {
    public:
        InstrumentationTimer(StringView name) : name_(name), stopped_(false)
            { start_time_point_ = std::chrono::steady_clock::now(); }

        ~InstrumentationTimer() { if(!stopped_) stop(); }

        void stop();
    private:
        StringView name_;
        std::chrono::steady_clock::time_point start_time_point_{};
        bool stopped_;
    };

    export namespace InstrumentorUtils
    {
        // static, to support "constexpr"
        template<Size N>
        struct ChangeResult
        {
            char8 data[N];
            Size size{ N };
        };

        // Used for cleaning "to-remove" str(__cdecl) in "expr"(function name)
        template<Size N, Size K>
        constexpr auto cleanup_output_string(const char8 (&expr)[N], const char8 (&remove)[K])
        {
            ChangeResult<N> result{};

            Size src_index{};
            Size dst_index{};
            while(src_index < N)
            {
                Size match_index{};
                while(match_index < K - 1 && src_index + match_index < N - 1 &&
                      expr[src_index + match_index] == remove[match_index]) match_index++;
                if(match_index == K - 1) src_index += match_index;

                // replace " with '(for json file)
                result.data[dst_index++] = expr[src_index] == '"' ? '\'' : expr[src_index];
                src_index++;
            }
            result.data[N - 1] = '\0'; // safe
            return result;
        }
    }
}
