module;
module UUID;

import Random;

namespace rke
{
    UUID::UUID()
        : val_(Random::pull_num<uint64>(1, UINT64_MAX)) {}
}
