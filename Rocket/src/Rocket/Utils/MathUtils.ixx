module;

#include <array>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module MathUtils;

import Types;

export namespace rke::math
{
    consteval uint32 bit(int x) { return (1U << x); }

    RKE_API std::array<glm::vec2, 4> calc_uv(glm::vec2 min, glm::vec2 max);

    RKE_API glm::vec4 srgb_to_linear(glm::vec4 color, float gamma = 2.2f);
    RKE_API glm::vec4 linear_to_srgb(glm::vec4 color, float gamma = 2.2f);
}
