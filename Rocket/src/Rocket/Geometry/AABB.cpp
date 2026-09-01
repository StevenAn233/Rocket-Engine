module;
module AABB;

namespace rke
{
    AABB::AABB(float angle, glm::vec2 size, glm::vec2 half_extent,
        glm::vec2 position, glm::vec2 offset)
    {
        const float rad{ glm::radians(angle) };
        const float c{ std::cos(rad) }, s{ std::sin(rad) };

        glm::vec2 h{ half_extent * size };
        if(angle != 0.0f) h = glm::vec2
        (
            std::abs(h.x * c) + std::abs(h.y * s),
            std::abs(h.x * s) + std::abs(h.y * c)
        );

        glm::vec2 centre{ position + glm::vec2
        (
            c * offset.x - s * offset.y,
            s * offset.x + c * offset.y
        )};

        min_ = centre - h;
        max_ = centre + h;
    }
}
