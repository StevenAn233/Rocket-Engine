module;
module MathUtils;

namespace {
    static constexpr std::array<glm::vec2, 4> s_default_uv
    {
        glm::vec2(1.0f, 1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f)
    };
}

namespace rke::math
{
    std::array<glm::vec2, 4> calc_uv(glm::vec2 min, glm::vec2 max)
    {
        return {
            glm::vec2(max.x, max.y),
            glm::vec2(min.x, max.y),
            glm::vec2(min.x, min.y),
            glm::vec2(max.x, min.y)
        };
    }

    std::array<glm::vec2, 4> calc_uv(Texture2D* texture,
        glm::vec2 coords, glm::vec2 cell_size, glm::vec2 cells)
    {
        if(!texture) return s_default_uv;
        glm::vec2 min {
            (coords.x * cell_size.x) / texture->get_width (),
            (coords.y * cell_size.y) / texture->get_height()
        };
        glm::vec2 max {
            ((coords.x + cells.x) * cell_size.x) / texture->get_width (),
            ((coords.y + cells.y) * cell_size.y) / texture->get_height()
        };
        return calc_uv(min, max);
    }

    ImVec4 srgb_to_linear(ImVec4 color, float gamma)
    {
        float r{ std::pow(color.x, gamma) };
        float g{ std::pow(color.y, gamma) };
        float b{ std::pow(color.z, gamma) };
        return ImVec4(r, g, b, color.w);
    }

    glm::vec4 srgb_to_linear(glm::vec4 color, float gamma)
    {
        float r{ std::pow(color.x, gamma) };
        float g{ std::pow(color.y, gamma) };
        float b{ std::pow(color.z, gamma) };
        return glm::vec4(r, g, b, color.w);
    }

    ImVec4 linear_to_srgb(ImVec4 color, float gamma)
    {
        float inv_gamma{ 1.0f / gamma };
        float r{ std::pow(color.x, inv_gamma) };
        float g{ std::pow(color.y, inv_gamma) };
        float b{ std::pow(color.z, inv_gamma) };
        return ImVec4(r, g, b, color.w);
    }

    glm::vec4 linear_to_srgb(glm::vec4 color, float gamma)
    {
        float inv_gamma{ 1.0f / gamma };
        float r{ std::pow(color.x, inv_gamma) };
        float g{ std::pow(color.y, inv_gamma) };
        float b{ std::pow(color.z, inv_gamma) };
        return glm::vec4(r, g, b, color.w);
    }
}
