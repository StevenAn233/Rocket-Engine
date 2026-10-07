module;
module Components;

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

    glm::mat4 TransformComponent::get_transform() const
    {
    // T(translation) * R * S * T(-anchor)
        return glm::translate(glm::mat4(1.0f), translation)
             * glm::mat4_cast(glm::quat(glm::radians(rotation)))
             * glm::scale(glm::mat4(1.0f), scale)
             * glm::translate(glm::mat4(1.0f), -anchor);
    }

    void TransformComponent::premultiply_by(const TransformComponent& other)
    {
    // T(ot)RoSoT(-oa) * T(t)RST(-a), using S*T(v) == T(S*v)*S and R*T(v) == T(R*v)*R:
    //     = T(ot + Ro*So*(t - oa)) * (Ro*R) * (So*S) * T(-a)
    // so this transform's own anchor survives, and `anchor` is never written.
        const glm::quat rot_other{ glm::quat(glm::radians(other.rotation)) };
        const glm::quat rot_self { glm::quat(glm::radians(rotation)) };

    // every right-hand side is read before its own member is written, so other == *this is fine
        translation = other.translation + rot_other * (other.scale * (translation - other.anchor));
    // through eulerAngles and back, because that is how the component stores a rotation
        rotation = glm::degrees(glm::eulerAngles(rot_other * rot_self));
        scale = other.scale * scale;
    }

    SpriteComponent::SpriteComponent() : quad(&s_quad) {}

    void AnimatorComponent::set_clip(StringView name)
    {
        Size count{ std::min(name.size(), clip_name_cap - 1) };
        std::memcpy(&clip[0], name.data(), count);
        clip[count] = u8'\0';
    }
}
