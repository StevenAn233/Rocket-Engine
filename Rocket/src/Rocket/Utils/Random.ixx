module;

#include <utility>
#include <random>
#include "rke_macros.h"

export module Random;

import Types;

export namespace rke
{
    class RKE_API Random
    {
    public:
        static void set_seed(int64 seed) { get_engine().seed(seed); }
        static void set_random_seed() { set_seed(get_random_uint64()); }
        static void clear_seed() { get_engine().seed(get_random_uint64()); }

        static bool probable(float probability)
        {
            if(probability <= 0.0f) return false;
            if(probability >= 1.0f) return true;

            auto num{ pull_num(0.0f, 1.0f) };
            return num < probability;
        }

        template<typename T>
        static T pull_num(T min, T max)
        {
            if(min > max) [[unlikely]] { std::swap(min, max); }
            if constexpr(std::is_integral_v<T>) {
                std::uniform_int_distribution<T> dis(min, max);
                return dis(get_engine());
            } else {
                std::uniform_real_distribution<T> dis(min, max);
                return dis(get_engine());
            }
        }
    private:
        static uint64 get_random_uint64();
        static std::mt19937_64& get_engine();
    };
}
