module;
module Time;

import Log;
import String;

namespace rke
{
    Timer::Timer()
        : last_time_point_(std::chrono::steady_clock::now())
        , last_elapsed_time_(0.0) {}

    void Timer::update()
    {
        auto current_time_point{ std::chrono::steady_clock::now() };
        last_elapsed_time_ = std::chrono::duration_cast
            <std::chrono::duration<double>>(current_time_point - last_time_point_);
        last_time_point_ = current_time_point;
    }

    Ticker::Ticker(uint32 tps, uint32 multiple)
        : sec_per_tick_(1.0 / tps)
        , max_accumulated_(multiple * sec_per_tick_) {}

    void Ticker::reset(uint32 tps, uint32 multiple)
    {
        sec_per_tick_ = 1.0 / tps;
        max_accumulated_ = multiple * sec_per_tick_;
        time_accumulator_ = 0.0;
    }
}
