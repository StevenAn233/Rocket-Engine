#version 450

#include "../include/binding_points.glsl"

layout(location = 0) in vec4 a_local_pos;
layout(location = 1) in vec2 a_uv;

layout(location = 2) in mat4 a_transform;
layout(location = 6) in vec4 a_color;
layout(location = 7) in vec2 a_uv_offset;
layout(location = 8) in vec2 a_uv_scale;
layout(location = 9 ) in int a_tex_id;
layout(location = 10) in int a_is_tex_grey;
layout(location = 11) in int a_entity_id;

layout(std140, binding = UBO_Camera) uniform Camera
{
	mat4 view_proj;
} u_camera;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_uv_coord;

layout(location = 2) out flat int v_tex_id;
layout(location = 3) out flat int v_is_tex_grey;
layout(location = 4) out flat int v_entity_id;

void main()
{
	vec4 world_pos = a_transform * a_local_pos;
	gl_Position = u_camera.view_proj * world_pos;

	v_color = a_color;
	v_uv_coord = a_uv * a_uv_scale + a_uv_offset;

	v_tex_id = a_tex_id;
	v_is_tex_grey = a_is_tex_grey;
	v_entity_id = a_entity_id;
}
