module;
module Plane;

import Log;

namespace rke
{
    PlaneBasis::PlaneBasis(glm::vec3 axis) : PlaneBasis(glm::vec4(axis, 0.0f)) {}

    PlaneBasis::PlaneBasis(glm::vec4 plane)
    {
        const glm::vec3 axis{ glm::vec3(plane) };
        const float len{ glm::length(axis) };
        if(len < 1e-6f) {
            CORE_WARN(u8"PlaneBasis: Axis length way too small!");
            return; // default value
        }
        normal_ = axis / len;
        offset_ = plane.w / len;

        const glm::vec3 ref{ std::abs(normal_.y) < 0.999f ?
            glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f) };

        u_ = glm::normalize(glm::cross(ref, normal_));
        v_ = glm::cross(normal_, u_);      
    }

    bool PlaneBasis::operator==(const PlaneBasis& other)
        { return (std::memcmp(this, &other, sizeof(PlaneBasis)) == 0); }

    float PlaneBasis::project_angle(glm::vec3 dir) const
    {
        const glm::vec2 pos{ to_uv(dir) };
        if(glm::dot(pos, pos) < 1e-8f) return 0.0f; // edge-on: no in-plane direction
        return glm::degrees(std::atan2(pos.y, pos.x));
    }

    float PlaneBasis::angle_of(glm::quat orientation) const
        { return project_angle(glm::mat3_cast(orientation) * glm::vec3(1.0f, 0.0f, 0.0f)); }

    glm::quat PlaneBasis::compose_spin(glm::quat orientation, float spin_degrees) const
        { return glm::angleAxis(glm::radians(spin_degrees), normal_) * orientation; }
}
