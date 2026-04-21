#version 450

#include "../include/binding_points.glsl"

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv_coord;

layout(binding = Sampler2D_0) uniform sampler2D u_scene_texture;
layout(binding = Sampler2D_1) uniform sampler2D u_silhouette_texture;

layout(std140, binding = UBO_PostProcess) uniform Uniforms
{
    vec4  outline_color;
    float thickness;
} u_data;

float get_edge_strength(vec2 uv, float thickness)
{
    float total_alpha_diff = 0.0;
    float samples = 0.0;

    vec4  offsets  = vec4(-1, 0, 1, 0) * thickness;
    ivec2 tex_size = textureSize(u_silhouette_texture, 0);
    vec2  texel_size   = 1.0 / vec2(tex_size);
    float center_alpha = texture(u_silhouette_texture, uv).r;
    for(int i = 0; i < 4; i++)
    {
        vec2 offset = vec2(offsets[i], offsets[(i + 1) % 4]) * texel_size;
        float neighbor_alpha = texture(u_silhouette_texture, uv + offset).r;
        total_alpha_diff += abs(center_alpha - neighbor_alpha);
        samples += 1.0;
    }

    if(total_alpha_diff / samples < 0.02) return 0.0;
   
    for(int i = 0; i < 4; i++)
    {
        vec2 offset = vec2(offsets[i], offsets[(i + 1) % 4]) * texel_size * 1.414;
        float neighbor_alpha = texture(u_silhouette_texture, uv + offset).r;
        total_alpha_diff += abs(center_alpha - neighbor_alpha);
        samples += 1.0;
    }
   
    return smoothstep(0.05, 0.15, total_alpha_diff / samples);
}

void main()
{
    vec4 scene_color = texture(u_scene_texture, v_uv_coord);
    float edge_strength = get_edge_strength(v_uv_coord, u_data.thickness);
    vec4 mixed_rgb = mix(scene_color, u_data.outline_color, edge_strength);
    o_color = vec4(mixed_rgb.rgb, 1.0);
}
