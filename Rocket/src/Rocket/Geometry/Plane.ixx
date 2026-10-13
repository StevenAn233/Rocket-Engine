module;

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "rke_macros.h"

export module Plane;

import Types;

export namespace rke
{
    class RKE_API Plane // inside world
    {
    public:
        Plane() = default;
        explicit Plane(glm::vec3 axis);
        explicit Plane(glm::vec4 def);

        Plane(const Plane&) = default;
        Plane& operator=(const Plane&) = default;
        Plane(Plane&&) = default;
        Plane& operator=(Plane&&) = default;

        bool operator==(const Plane& other);

        inline void add_distance(float addon ) { distance_ += addon; }
        inline void set_distance(float offset) { distance_ = offset; }

        inline glm::vec3 get_u() const { return u_; }
        inline glm::vec3 get_v() const { return v_; }
        inline glm::vec3 get_normal() const { return normal_; }
        inline float get_distance() const { return distance_; }

        inline glm::vec4 get_define() const { return { normal_, distance_ }; }
        inline glm::mat3 get_basis() const { return glm::mat3(u_, v_, normal_); }

        inline glm::vec2 to_uv(glm::vec3 world) const
            { return { glm::dot(world, u_), glm::dot(world, v_) }; }
        inline glm::vec3 to_world(glm::vec2 uv) const
            { return (u_ * uv.x + v_ * uv.y) - (normal_ * distance_); }

        inline float distance_to(glm::vec3 point) const
            { return glm::dot(normal_, point) + distance_; }

        float project_angle(glm::vec3 dir) const;
        float angle_of(glm::quat orientation) const;
        glm::quat compose_spin(glm::quat orientation, float spin_degrees) const;
    private:
        glm::vec3 u_{ 1.0f, 0.0f, 0.0f }; // u axis itself, a vector in world
        glm::vec3 v_{ 0.0f, 1.0f, 0.0f }; // v axis itself, a vector in world
        glm::vec3 normal_{ 0.0f, 0.0f, 1.0f };
        float distance_{ 0.0f }; // distance between plane and world origin
    };

    using Frustum = std::array<Plane, 6>;

    class RKE_API PlaneBox // inside plane
    {
    public:
        PlaneBox() = default;
        PlaneBox(glm::vec2 centre, glm::vec2 size, float rotation = 0.0f);

        inline glm::vec2 get_centre() const { return centre_; }
        inline glm::vec2 get_size() const { return size_; }
        inline float get_rotation() const { return rotation_; }

        inline glm::vec2 get_half_size() const { return 0.5f * size_; }
        inline bool empty() const { return size_.x <= 0.0f || size_.y <= 0.0f; }

    // relative to its plane
        glm::mat3 get_transform() const;
        glm::mat2 get_basis() const;
        glm::mat4 to_world(const Plane& plane) const;

    // four corners(points in plane uv), counter-clockwise starting at (-u, -v)
        std::array<glm::vec2, 4> get_corners() const;
        float get_radius() const;

    // axis-aligned 2D envelope of this rotated box, in plane space
        glm::vec2 get_envelope_half_size() const;
        inline glm::vec2 get_min() const { return centre_ - get_envelope_half_size(); }
        inline glm::vec2 get_max() const { return centre_ + get_envelope_half_size(); }

        bool contains(glm::vec2 point) const;
        bool overlaps(const PlaneBox& other) const; // separating axis test
    private:
        glm::vec2 centre_{ 0.0f };
        glm::vec2 size_  { 0.0f };
        float rotation_  { 0.0f }; // degree
    };
}
