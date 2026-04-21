module;
module Instrumentor;

import Log;

namespace rke
{
    Instrumentor::Instrumentor () {}
    Instrumentor::~Instrumentor() { end_session(); }

    void Instrumentor::begin_session(const String& name, const Path& filepath)
    {
        std::lock_guard lock{ mutex_ };
        if(current_session_) { // there had already begun a session
            // If there is already a current session, then close it before beginning new one.
            // Subsequent profiling output meant for the original session will end up in the
            // newly opened session instead.  That's better than having badly formatted
            // profiling output.
            CORE_ERROR (
                u8"Instrumentor: BeginSession('{}') called with session '{}' already open.",
                name, current_session_->name);
            internal_end_session();
        }
        fout_.open(filepath.get());

        if(fout_.is_open()) {
            current_session_    = new InstrumentationSession({name});
            is_session_active_  = true;
            session_start_time_ = std::chrono::steady_clock::now();

            write_header();
        }
        else CORE_ERROR(u8"Instrumentor: Could not open results file '{}'!", filepath);
    }

    void Instrumentor::internal_end_session()
    {
        if(current_session_)
        {
            is_session_active_ = false;
            write_footer();
            fout_.close ();
            delete current_session_;
            current_session_ = nullptr;
        }
    }

    void Instrumentor::end_session()
    {
        std::lock_guard lock{ mutex_ };
        internal_end_session();
    }

    float Instrumentor::get_session_duration_s() const
    {
        if(!is_session_active_) return 0.0f;
        auto elapsed_time{ std::chrono::steady_clock::now() - session_start_time_ };
        return std::chrono::duration_cast<std::chrono::duration<float>>(elapsed_time).count();
    }

    void Instrumentor::write_profile(const ProfileResult& result)
    {
        std::lock_guard lock(mutex_);
        if(current_session_)
        {
            std::ostringstream json{};

            json << std::setprecision(3) << std::fixed;
            json << ",{";
            json << "\"cat\":\"function\",";
            json << "\"dur\":"	  << result.elapsed_time.count() << ',';
            json << "\"name\":\"" << result.name.raw_unsafe() << "\",";
            json << "\"ph\":\"X\",";
            json << "\"pid\":0,";
            json << "\"tid\":"	  << result.thread_id << ",";
            json << "\"ts\":"	  << result.start.count();
            json << "}";

            fout_ << json.str();
            fout_.flush();
        }
    }

    void InstrumentationTimer::stop()
    {
        auto end_time_point { std::chrono::steady_clock::now() };
        auto high_res_start {
            std::chrono::duration_cast<std::chrono::duration<double, std::micro>>
                (start_time_point_.time_since_epoch())
        };
        std::chrono::microseconds elapsed_time {
            std::chrono::duration_cast<std::chrono::microseconds>
                (end_time_point - start_time_point_)
        };

        Instrumentor::get().write_profile
            ({ name_, high_res_start, elapsed_time, std::this_thread::get_id()});

        stopped_ = true;
    }
}
