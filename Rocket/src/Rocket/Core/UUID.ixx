module;

#include <functional>
#include "rke_macros.h"

export module UUID;

import Types;

export namespace rke
{
    class RKE_API UUID
    {
    public:
        UUID();
        UUID(uint64 val) : val_(val) {}
        UUID(const UUID& other) : val_(other.val_) {}

        operator uint64() const { return val_; }
        uint64 value() const { return val_; }
        bool empty() const { return val_ == 0; }

        bool operator==(const UUID& other) const { return val_ == other.val_; }
        bool operator!=(const UUID& other) const { return val_ != other.val_; }
    private:
        uint64 val_; // 0 for invalid/empty
    };
}

namespace std
{
    export template<>
    struct hash<rke::UUID> {
        size_t operator()(const rke::UUID& uuid) const
            { return std::hash<std::uint64_t>{}(uuid.value()); }
    };
}
