#version 450

#include "../include/binding_points.glsl"

layout(location = 0) in vec2 v_uv_coord;

layout(location = 0) out vec4 o_color;

layout(binding = Sampler2D_0) uniform sampler2D u_source;
layout(binding = Sampler2D_1) uniform sampler2D u_outline;

void main()
{
    vec3 source_col = texture(u_source , v_uv_coord).rgb;
    vec4 outline_col = texture(u_outline, v_uv_coord);
    
    vec3 blended = (outline_col.rgb * outline_col.a) + source_col * (1.0 - outline_col.a);
    o_color = vec4(blended, 1.0);
}
