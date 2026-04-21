module;

#include <array>
#include <imgui.h>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module MathUtils;

import HeapManager;
import Texture;

export namespace rke::math
{
    RKE_API std::array<glm::vec2, 4> calc_uv(glm::vec2 min, glm::vec2 max);
    RKE_API std::array<glm::vec2, 4> calc_uv(Texture2D* texture,
        glm::vec2 coords, glm::vec2 cell_size, glm::vec2 cells = { 1, 1 });

    RKE_API ImVec4    srgb_to_linear(ImVec4    color, float gamma = 2.2f);
    RKE_API glm::vec4 srgb_to_linear(glm::vec4 color, float gamma = 2.2f);
    RKE_API ImVec4    linear_to_srgb(ImVec4    color, float gamma = 2.2f);
    RKE_API glm::vec4 linear_to_srgb(glm::vec4 color, float gamma = 2.2f);
}
