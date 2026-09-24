module;

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
export module Plane;

import Types;

export namespace rke
{
    inline glm::vec3 plane_axis() { return glm::vec3(0.0f, 0.0f, 1.0f); } // XOY, hard-coded for now

    class PlaneBasis
    {
    public:
        PlaneBasis() = default;
        PlaneBasis(glm::vec3 axis);

        inline glm::vec3 get_normal() const { return normal_; }
        inline glm::vec3 get_u() const { return u_; }
        inline glm::vec3 get_v() const { return v_; }
        inline glm::mat3 get_mat() const { return glm::mat3(u_, v_, normal_); }

        // world -> plane (2D collider space)
        inline glm::vec2 to_plane(glm::vec3 pos) const
            { return { glm::dot(pos, u_), glm::dot(pos, v_) }; }

        // plane (2D collider space) -> world
        inline glm::vec3 from_plane(glm::vec2 pos) const
            { return u_ * pos.x + v_ * pos.y; }

    // In-plane angle of a world-space direction, in degrees. The ONLY way a solver's
    // single angle is read back out of a full orientation, so every caller goes here.
        float project_plane_angle(glm::vec3 dir) const;

    // Total in-plane angle of an orientation. A tilt about the normal or about an
    // in-plane axis leaves this direction intact (the latter only scales it), so this
    // recovers the spin a solver applied on top of any authored tilt.
        float plane_angle_of(glm::quat orientation) const;

    // Compose a spin onto an orientation: a world-space turn about the plane normal.
    // It commutes with any tilt about that same normal, which is exactly why a tilt
    // survives a physics step that is folded in this way.
        glm::quat compose_plane_spin(glm::quat orientation, float spin_degrees) const;
    private:
        glm::vec3 u_{ 1.0f, 0.0f, 0.0f };
        glm::vec3 v_{ 0.0f, 1.0f, 0.0f };
        glm::vec3 normal_{ 0.0f, 0.0f, 1.0f }; // the XOY plane by default
    };
}
