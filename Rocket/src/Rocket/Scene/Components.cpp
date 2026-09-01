module;
module Components;

import HeapManager;

namespace rke
{
    static const Mesh s_quad ( // may modify
        4, 6,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
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

    SpriteComponent::SpriteComponent(AssetUUID uuid)
        : tex_uuid(uuid), quad(&s_quad) {}
}
