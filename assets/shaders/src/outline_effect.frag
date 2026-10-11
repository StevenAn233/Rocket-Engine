#version 450

#include "../include/binding_points.glsl"

layout(std140, binding = UBO_PostProcess)
uniform Uniforms { vec4 color; } u_data;

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = u_data.color;
}
