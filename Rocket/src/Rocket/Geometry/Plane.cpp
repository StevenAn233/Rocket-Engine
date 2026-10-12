module;
module Plane;

import Log;

namespace rke
{
    Plane::Plane(glm::vec3 axis) : Plane(glm::vec4(axis, 0.0f)) {}

    Plane::Plane(glm::vec4 def)
    {
        const glm::vec3 axis{ glm::vec3(def) };
        const float len{ glm::length(axis) };
        if(len < 1e-6f) {
            CORE_WARN(u8"Plane: Axis length way too small!");
            return; // default value
        }
        normal_ = axis / len;
        distance_ = def.w / len;

        const glm::vec3 flat_x{ glm::vec3(1.0f, 0.0f, 0.0f) - normal_ * normal_.x };
        const glm::vec3 flat_y{ glm::vec3(0.0f, 1.0f, 0.0f) - normal_ * normal_.y };

        u_ = glm::normalize(glm::dot(flat_x, flat_x) > 1e-6f ? flat_x : flat_y);
        v_ = glm::cross(normal_, u_);
    }
    
    bool Plane::operator==(const Plane& other)
        { return (std::memcmp(this, &other, sizeof(Plane)) == 0); }

    float Plane::project_angle(glm::vec3 dir) const
    {
        const glm::vec2 pos{ to_uv(dir) };
        if(glm::dot(pos, pos) < 1e-8f) return 0.0f; // edge-on: no in-plane direction
        return glm::degrees(std::atan2(pos.y, pos.x));
    }

    float Plane::angle_of(glm::quat orientation) const
        { return project_angle(glm::mat3_cast(orientation) * glm::vec3(1.0f, 0.0f, 0.0f)); }

    glm::quat Plane::compose_spin(glm::quat orientation, float spin_degrees) const
        { return glm::angleAxis(glm::radians(spin_degrees), normal_) * orientation; }

// PlaneBox
    PlaneBox::PlaneBox(glm::vec2 centre, glm::vec2 size, float rotation)
        : centre_(centre), size_(size), rotation_(rotation) {}

    glm::mat3 PlaneBox::get_transform() const
    {
        const float rad{ glm::radians(rotation_) };
        return glm::mat3 {
            std::cos(rad) * size_.x, std::sin(rad) * size_.x, centre_.x,
           -std::sin(rad) * size_.y, std::cos(rad) * size_.y, centre_.y,
            0.0f, 0.0f, 1.0f
        };
    }

    glm::mat2 PlaneBox::get_basis() const
        { return glm::mat2(get_transform()); }

    std::array<glm::vec2, 4> PlaneBox::get_corners() const
    {
        const glm::mat2 basis{ get_basis() };
        const glm::vec2 u{ basis[0] * 0.5f }; // signed
        const glm::vec2 v{ basis[1] * 0.5f }; // signed
        return { centre_ - u - v, centre_ + u - v, centre_ + u + v, centre_ - u + v };
    }

    float PlaneBox::get_radius() const { return glm::length(get_half_size()); }

    glm::vec2 PlaneBox::get_envelope_half_size() const
    {
        const glm::mat2 basis{ get_basis() };
        const glm::vec2 u{ glm::abs(basis[0]) * 0.5f };
        const glm::vec2 v{ glm::abs(basis[1]) * 0.5f };
        return u + v;
    }

    bool PlaneBox::contains(glm::vec2 point) const
    {
        if(empty()) return false;
        const glm::vec2 local{ glm::inverse(get_basis()) * (point - centre_) };
        return std::abs(local.x) <= 0.5f && std::abs(local.y) <= 0.5f;
    }

    bool PlaneBox::overlaps(const PlaneBox& other) const
    {
        if(empty() || other.empty()) return false;

        const glm::mat2 basis_a{ get_basis() };
        const glm::mat2 basis_b{ other.get_basis() };
        const glm::vec2 delta{ other.get_centre() - centre_ };

        const glm::vec2 axes[4]
            { basis_a[0], basis_a[1], basis_b[0], basis_b[1] };
        for(glm::vec2 axis : axes)
        {
            const float gap{ std::abs(glm::dot(delta, axis)) };
            const float reach
            {
                (std::abs(glm::dot(basis_a[0], axis)) + std::abs(glm::dot(basis_a[1], axis)) +
                 std::abs(glm::dot(basis_b[0], axis)) + std::abs(glm::dot(basis_b[1], axis))) * 0.5f
            };
            if(gap > reach) return false; // separated along this axis
        }
        return true;
    }
}
