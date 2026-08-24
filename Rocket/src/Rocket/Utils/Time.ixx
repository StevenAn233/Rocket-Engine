module;

#include <chrono>
#include <concepts>
#include <functional>
#include "rke_macros.h"

export module Time;

import Types;

export namespace rke
{
    class RKE_API Timer
    {
    public:
        Timer();
        virtual ~Timer() = default;

        void update();
        inline double get_last_elapsed() const
            { return last_elapsed_time_.count(); }
    private:
        std::chrono::steady_clock::time_point last_time_point_;
        std::chrono::duration<double> last_elapsed_time_;
    };

    class RKE_API Ticker
    {
    public:
        Ticker(uint32 tps) : sec_per_tick_(1.0 / tps) {}
        inline void reset(uint32 tps) { sec_per_tick_ = 1.0 / tps; }

        template<typename Func> // void func(double delta_time)
        requires std::invocable<Func, double>
        inline void tick(double addon_time, Func&& func)
        {
            time_accumulator_ += addon_time;
            if(time_accumulator_ > sec_per_tick_)
            {
                time_accumulator_ -= sec_per_tick_;
                std::invoke(std::forward<Func>(func), sec_per_tick_);
            }
        }
    private:
        double sec_per_tick_; // second
        double time_accumulator_{}; // second
    };
}
