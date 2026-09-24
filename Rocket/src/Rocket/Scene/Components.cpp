module;
module Components;

namespace rke
{
    static const Mesh s_quad ( // may modify
        4, 6,
        glm::vec3(0.0f, 0.0f, 1.0f), // front
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

    glm::mat4 TransformComponent::get_transform(glm::vec3 mesh_centre) const
    {
        return glm::translate(glm::mat4(1.0f), translation)
             * glm::mat4_cast(glm::quat(glm::radians(rotation)))
             * glm::scale(glm::mat4(1.0f), scale)
             * glm::translate(glm::mat4(1.0f), -mesh_centre);
    }

    SpriteComponent::SpriteComponent() : quad(&s_quad) {}

    void AnimatorComponent::set_clip(StringView name)
    {
        Size count{ std::min(name.size(), clip_name_cap - 1) };
        std::memcpy(&clip[0], name.data(), count);
        clip[count] = u8'\0';
    }
}
