module;

#include <cmath>
#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

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
        const glm::mat3 upper {
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
        const float size{ std::cbrt(std::abs(glm::determinant(glm::mat3(mat)))) };
        if(size < 1e-30f)
        {
            CORE_WARN(u8"Entity: Transform is scaled to zero, can't set it!");
            return;
        }

        glm::mat4 unit{ mat };
        for(int i = 0; i < 3; ++i) unit[i] /= size;

        glm::vec3 new_scale{}, skew{}, unused_translation{};
        glm::vec4 perspective{}; glm::quat new_rotation{};
        if(!glm::decompose(unit, new_scale, new_rotation,
            unused_translation, skew, perspective))
        {
            CORE_WARN(u8"Entity: Transform can't be factorized, can't set it!");
            return;
        }

        rotation = new_rotation;
        scale = new_scale * size;
        shear = glm::vec3{ skew.z, skew.y, skew.x };
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
