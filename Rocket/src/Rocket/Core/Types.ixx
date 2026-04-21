module;

#include <cstdint>
#include <cinttypes>
#include <string>

export module Types;

export namespace rke
{
    using uint8   = uint8_t;
    using uint16  = uint16_t;
    using uint32  = uint32_t;
    using uint64  = uint64_t;
    using uintptr = uintptr_t;

    using int8  = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;
    static_assert(sizeof(int) == 4, u8"Rocket Engine: Type 'int' has to be 32 bits!");

    using char8 = char8_t;
    using byte  = unsigned char;

    using Size = size_t;

    using CharBuffer = std::string;
}
