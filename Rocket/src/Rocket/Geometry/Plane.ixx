module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "rke_macros.h"

export module Plane;

import Types;

export namespace rke
{
    class RKE_API PlaneBasis
    {
    public:
        PlaneBasis() = default;
        explicit PlaneBasis(glm::vec3 axis );
        explicit PlaneBasis(glm::vec4 plane);

        PlaneBasis(const PlaneBasis&) = default;
        PlaneBasis& operator=(const PlaneBasis&) = default;
        PlaneBasis(PlaneBasis&&) = default;
        PlaneBasis& operator=(PlaneBasis&&) = default;

        bool operator==(const PlaneBasis& other);

        inline void add_offset(float addon ) { offset_ += addon; }
        inline void set_offset(float offset) { offset_ = offset; }

        inline glm::vec3 get_u() const { return u_; }
        inline glm::vec3 get_v() const { return v_; }
        inline glm::vec3 get_normal() const { return normal_; }
        inline float get_offset() const { return offset_; }

        inline glm::vec4 get_vec4() const { return { normal_, offset_ }; }
        inline glm::mat3 get_mat() const { return glm::mat3(u_, v_, normal_); }

        inline glm::vec2 to_uv(glm::vec3 world) const
            { return { glm::dot(world, u_), glm::dot(world, v_) }; }
        inline glm::vec3 to_world(glm::vec2 uv) const
            { return u_ * uv.x + v_ * uv.y; }

        inline float signed_distance(glm::vec3 point) const
            { return glm::dot(normal_, point) + offset_; }

        float project_angle(glm::vec3 dir) const;
        float angle_of(glm::quat orientation) const;
        glm::quat compose_spin(glm::quat orientation, float spin_degrees) const;
    private:
        glm::vec3 u_{ 1.0f, 0.0f, 0.0f }; // u axis itself, a vector in world
        glm::vec3 v_{ 0.0f, 1.0f, 0.0f }; // v axis itself, a vector in world
        glm::vec3 normal_{ 0.0f, 0.0f, 1.0f };
        float offset_{ 0.0f }; // distance between plane and world origin
    };
}
