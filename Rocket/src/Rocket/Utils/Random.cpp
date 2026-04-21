module;
module Random;

namespace rke
{
    uint64 Random::get_random_uint64()
    {
        static std::random_device rd{};
        return (static_cast<uint64>(rd()) << 32) | rd();
    }

    std::mt19937_64& Random::get_engine()
    {
        thread_local std::mt19937_64 engine(get_random_uint64());
        return engine;
    }
}
