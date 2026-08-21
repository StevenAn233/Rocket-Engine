module;
module Random;

namespace rke
{
    uint64 Random::get_random_uint64()
    {
        static std::random_device s_device{};
        return (static_cast<uint64>(s_device()) << 32) | s_device();
    }

    std::mt19937_64& Random::get_engine()
    {
        thread_local std::mt19937_64 s_engine{ get_random_uint64() };
        return s_engine;
    }
}
