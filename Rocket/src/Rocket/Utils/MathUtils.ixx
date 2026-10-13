module;

#include <array>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module MathUtils;

import Types;

export namespace rke::math
{
    consteval uint32 bit(int x) { return (1U << x); }
    RKE_API glm::vec4 srgb_to_linear(glm::vec4 color, float gamma = 2.2f);
    RKE_API glm::vec4 linear_to_srgb(glm::vec4 color, float gamma = 2.2f);


    RKE_API glm::vec2 world_to_viewport
        (const glm::mat4& view_proj, glm::vec3 world, glm::vec2 viewport_size);

    RKE_API glm::vec2 world_to_screen
        (const glm::mat4& view_proj, glm::vec3 world, glm::vec2 viewport_size);

    RKE_API bool is_in_front(const glm::mat4& view_proj, glm::vec3 world);
}
