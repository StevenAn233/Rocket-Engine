module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module AABB;

export namespace rke
{
    class RKE_API AABB
    {
    public:
        AABB() {}
        AABB(float angle, glm::vec2 size, glm::vec2 half_extent,
            glm::vec2 position, glm::vec2 offset);
        ~AABB() = default;

        inline glm::vec2 get_centre() const { return 0.5f * (min_ + max_); }
        inline glm::vec2 get_size() const { return max_ - min_; }

        inline bool contains(glm::vec2 p) const
            { return p.x >= min_.x && p.x <= max_.x && p.y >= min_.y && p.y <= max_.y; }
        inline bool overlaps(const AABB& other) const
        {
            return min_.x <= other.max_.x && other.min_.x <= max_.x &&
                   min_.y <= other.max_.y && other.min_.y <= max_.y;
        }
    private:
        glm::vec2 min_{ 0.0f };
        glm::vec2 max_{ 0.0f };
    };
}
