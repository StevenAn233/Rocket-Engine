#version 450

#include "../include/binding_points.glsl"

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv_coord;

layout(binding = Sampler2D_0) uniform sampler2D u_texture;

layout(std140, binding = UBO_PostProcess) uniform Uniforms
{
    float exposure;
    float gamma;
} u_data;

vec3 ACES_film(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() 
{
    vec4 hdr_color = texture(u_texture, v_uv_coord);
    vec3 color = hdr_color.rgb;

    color *= u_data.exposure;

    // Tone Mapping (HDR Linear -> LDR Linear)
    color = ACES_film(color);

    // LDR Linear -> sRGB
    color = pow(color, vec3(1.0 / u_data.gamma));

    o_color = vec4(color, hdr_color.a);
}
