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

    // Reference axis chosen so cross(ref, normal) lands on the world X axis for XOY:
    // cross((0,1,0),(0,0,1)) == (1,0,0) -> u=(1,0,0)/v=(0,1,0), so the projection is an
    // IDENTITY on that plane. Rotating this choice by 180 degrees still yields an
    // orthonormal, right-handed basis -- it just comes out rotated 90 degrees, which
    // silently shifts EVERY plane angle and makes the solver chase a non-zero delta
    // forever. It must also never be parallel to the normal.
        const glm::vec3 ref{ std::abs(normal_.y) < 0.999f
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f) };
        u_ = glm::normalize(glm::cross(ref, normal_));
        v_ = glm::cross(normal_, u_);
    }

    float PlaneBasis::project_plane_angle(glm::vec3 dir) const
    {
        const glm::vec2 pos{ glm::dot(dir, u_), glm::dot(dir, v_) };
        if(glm::dot(pos, pos) < 1e-8f) return 0.0f; // edge-on: no in-plane direction
        return glm::degrees(std::atan2(pos.y, pos.x));
    }

    float PlaneBasis::plane_angle_of(glm::quat orientation) const
    {
        return project_plane_angle
            (glm::mat3_cast(orientation) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::quat PlaneBasis::compose_plane_spin(glm::quat orientation, float spin_degrees) const
        { return glm::angleAxis(glm::radians(spin_degrees), normal_) * orientation; }
}
