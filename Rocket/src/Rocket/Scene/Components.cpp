module;
module Components;

import Log;

namespace rke
{
    static const Mesh s_quad ( // may modify
        4, 6,
        Scope<glm::vec4[]>(new glm::vec4[4]
        {
            glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
            glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f)
        }),
        Scope<uint32[]>(new uint32[6]{ 0, 1, 2, 2, 3, 0 }),
        nullptr, nullptr,
        Scope<glm::vec2[]>(new glm::vec2[4]
        {
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f)
        })
    );

    IdentityComponent::IdentityComponent()
        : tag({}), uuid() { std::memcpy(&tag[0], u8"Null", 4); }

    IdentityComponent::IdentityComponent(const char8* str, UUID uuid)
        : tag({}), uuid(uuid) { set_tag(StringView(str)); }

    void IdentityComponent::set_tag(StringView new_tag)
    {
        Size count{ std::min(new_tag.size(), tag_size - 1) };
        std::memcpy(&tag[0], new_tag.data(), count);
        tag[count] = u8'\0';
    }

    glm::mat4 TransformComponent::get_mat() const
    {
        const glm::mat3 upper
        {
            scale.x,           0.0f,              0.0f,
            scale.y * shear.x, scale.y,           0.0f,
            scale.z * shear.y, scale.z * shear.z, scale.z
        };
        return glm::translate(glm::mat4(1.0f), translation)
             * glm::mat4_cast(rotation) * glm::mat4(upper)
             * glm::translate(glm::mat4(1.0f), -anchor);
    }

    void TransformComponent::set_to(const glm::mat4& mat)
    {
        glm::mat3 axis{ glm::mat3(mat) };
        glm::vec3 axis_scale
            { glm::length(axis[0]), glm::length(axis[1]), glm::length(axis[2]) };
        if(axis_scale.x < 1e-6f || axis_scale.y < 1e-6f || axis_scale.z < 1e-6f)
        {
            CORE_WARN(u8"Entity: World transform is scaled to zero, can't set it!");
            return;
        }

    // a non-uniform scale above a rotation shears the frame; that shear goes into tc.shear and
    // the axes are orthonormalized, so local factors into T(translation) * R * K * S * T(-anchor)
    // exactly, instead of coming back turned and stretched
        axis[0] /= axis_scale.x;

        float shear_xy{ glm::dot(axis[0], axis[1]) };
        axis[1] -= axis[0] * shear_xy;
        axis_scale.y = glm::length(axis[1]);
        shear_xy /= axis_scale.y;
        axis[1] /= axis_scale.y;

    // each shear is the dot with the axis that is still un-orthogonalized
        const float xz_dot{ glm::dot(axis[0], axis[2]) };
        const float yz_dot{ glm::dot(axis[1], axis[2]) };
        axis[2] -= axis[0] * xz_dot;
        axis[2] -= axis[1] * yz_dot;
        axis_scale.z = glm::length(axis[2]);
        const float shear_xz{ xz_dot / axis_scale.z };
        const float shear_yz{ yz_dot / axis_scale.z };
        axis[2] /= axis_scale.z;

    // a mirrored frame needs one negative scale, or quat_cast() can't return a rotation
        if(glm::dot(glm::cross(axis[0], axis[1]), axis[2]) < 0.0f)
        {
            axis = -axis;
            axis_scale = -axis_scale;
        }

        rotation = glm::quat_cast(axis);
        scale = axis_scale;
        shear = glm::vec3{ shear_xy, shear_xz, shear_yz };
        translation = glm::vec3(mat * glm::vec4(anchor, 1.0f));
    }

    SpriteComponent::SpriteComponent() : quad(&s_quad) {}

    void AnimatorComponent::set_clip(StringView name)
    {
        Size count{ std::min(name.size(), clip_name_cap - 1) };
        std::memcpy(&clip[0], name.data(), count);
        clip[count] = u8'\0';
    }
}
