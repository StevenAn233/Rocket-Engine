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
        ~Timer() = default;

        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;

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
        Ticker(uint32 tps, uint32 multiple = 4);
        ~Ticker() = default;
        
        Ticker(const Ticker&) = delete;
        Ticker& operator=(const Ticker&) = delete;
        Ticker(Ticker&&) = delete;
        Ticker& operator=(Ticker&&) = delete;

        void reset(uint32 tps, uint32 multiple = 4);

        template<typename Func> // void func(double delta_time)
        requires std::invocable<Func, double>
        inline void tick(double addon_time, Func&& func)
        {
            time_accumulator_ += addon_time;
            if(time_accumulator_ > max_accumulated_)
                time_accumulator_ = max_accumulated_;
            while(time_accumulator_ > sec_per_tick_)
            {
                time_accumulator_ -= sec_per_tick_;
                std::invoke(std::forward<Func>(func), sec_per_tick_);
            }
        }
    private:
        double sec_per_tick_;
        double max_accumulated_;
        double time_accumulator_{};
    };
}
