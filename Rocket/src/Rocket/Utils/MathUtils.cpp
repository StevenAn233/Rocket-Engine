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
}
