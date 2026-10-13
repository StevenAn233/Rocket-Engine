module;
module MathUtils;

namespace rke::math
{
    glm::vec4 srgb_to_linear(glm::vec4 color, float gamma)
    {
        float r{ std::pow(color.x, gamma) };
        float g{ std::pow(color.y, gamma) };
        float b{ std::pow(color.z, gamma) };
        return glm::vec4(r, g, b, color.w);
    }

    glm::vec4 linear_to_srgb(glm::vec4 color, float gamma)
    {
        float inv_gamma{ 1.0f / gamma };
        float r{ std::pow(color.x, inv_gamma) };
        float g{ std::pow(color.y, inv_gamma) };
        float b{ std::pow(color.z, inv_gamma) };
        return glm::vec4(r, g, b, color.w);
    }

    glm::vec2 world_to_viewport(const glm::mat4& view_proj,
        glm::vec3 world, glm::vec2 viewport_size)
    {
        const glm::vec4 clip{ view_proj * glm::vec4(world, 1.0f) };
        if(clip.w == 0.0f) return glm::vec2(0.0f); // degenerate: nothing to project

        const glm::vec2 ndc{ clip.x / clip.w, clip.y / clip.w }; // [-1, 1]
        return glm::vec2
        (
            (ndc.x * 0.5f + 0.5f) * viewport_size.x,
            (ndc.y * 0.5f + 0.5f) * viewport_size.y
        );
    }

    glm::vec2 world_to_screen(const glm::mat4& view_proj,
        glm::vec3 world, glm::vec2 viewport_size)
    {
        const glm::vec2 viewport{ world_to_viewport(view_proj, world, viewport_size) };
        return glm::vec2(viewport.x, viewport_size.y - viewport.y);
    }

    bool is_in_front(const glm::mat4& view_proj, glm::vec3 world)
        { return (view_proj * glm::vec4(world, 1.0f)).w > 0.0f; }
}
