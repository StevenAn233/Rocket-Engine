#version 450

#include "../include/binding_points.glsl"

layout(location = 0) in vec2 v_uv_coord;

layout(location = 0) out vec4 o_color;

layout(binding = Sampler2D_0) uniform sampler2D u_source;

void main()
{
    o_color = vec4(texture(u_source, v_uv_coord).rgb, 1.0);
}
