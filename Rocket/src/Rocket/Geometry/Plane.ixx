module;

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
export module Plane;

import Types;

export namespace rke
{
    class PlaneBasis
    {
    public:
        PlaneBasis() = default;
        explicit PlaneBasis(glm::vec3 axis);

        inline glm::vec3 get_u() const { return u_; }
        inline glm::vec3 get_v() const { return v_; }
        inline glm::vec3 get_normal() const { return normal_; }
        inline glm::mat3 get_mat() const { return glm::mat3(u_, v_, normal_); }

        inline glm::vec2 to_uv(glm::vec3 world) const
            { return { glm::dot(world, u_), glm::dot(world, v_) }; }
        inline glm::vec3 to_world(glm::vec2 uv) const
            { return u_ * uv.x + v_ * uv.y; }

        float project_angle(glm::vec3 dir) const;
        float angle_of(glm::quat orientation) const;
        glm::quat compose_spin(glm::quat orientation, float spin_degrees) const;
    private:
        glm::vec3 u_{ 1.0f, 0.0f, 0.0f }; // u axis itself, a vector in world
        glm::vec3 v_{ 0.0f, 1.0f, 0.0f }; // v axis itself, a vector in world
        glm::vec3 normal_{ 0.0f, 0.0f, 1.0f };
    };
}
