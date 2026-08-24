module;
module Time;

import String;

namespace rke
{
    Timer::Timer() : last_time_point_(std::chrono::steady_clock::now())
                   , last_elapsed_time_(0.0) {}

    void Timer::update()
    {
        auto current_time_point{ std::chrono::steady_clock::now() };
        last_elapsed_time_ = std::chrono::duration_cast
            <std::chrono::duration<double>>(current_time_point - last_time_point_);
        last_time_point_ = current_time_point;
    }
}
