module;

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

module Plane;

namespace rke
{
    PlaneBasis::PlaneBasis(glm::vec3 axis)
    {
        const float len{ glm::length(axis) };
        if(len < 1e-6f) return; // degenerate: keep XOY
        normal_ = axis / len;

        const glm::vec3 ref{ std::abs(normal_.y) < 0.999f ?
            glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f) };

    // uv and world(coord system) share the same origin
        u_ = glm::normalize(glm::cross(ref, normal_));
        v_ = glm::cross(normal_, u_);
    }

    float PlaneBasis::project_angle(glm::vec3 dir) const
    {
        const glm::vec2 pos{ to_uv(dir) };
        if(glm::dot(pos, pos) < 1e-8f) return 0.0f; // edge-on: no in-plane direction
        return glm::degrees(std::atan2(pos.y, pos.x));
    }

    float PlaneBasis::angle_of(glm::quat orientation) const
    {
        return project_angle (
            glm::mat3_cast(orientation) *
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
    }

    glm::quat PlaneBasis::compose_spin(glm::quat orientation, float spin_degrees) const
    {
        return glm::angleAxis
            (glm::radians(spin_degrees), normal_)
        * orientation;
    }
}
