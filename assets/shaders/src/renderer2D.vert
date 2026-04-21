#version 450

#include "../include/binding_points.glsl"

layout(location = 0) in vec3  a_position;
layout(location = 1) in vec4  a_color;
layout(location = 2) in vec2  a_uv_coord;
layout(location = 3) in float a_tiling_factor;

layout(location = 4) in int a_tex_id;
layout(location = 5) in int a_if_tex_grey;
layout(location = 6) in int a_is_font;
layout(location = 7) in int a_entity_id; // EDITOR ONLY

layout(std140, binding = UBO_Camera) uniform Camera
{
	mat4 view_proj;
} u_camera;

layout(location = 0) out vec4  v_color;
layout(location = 1) out vec2  v_uv_coord;
layout(location = 2) out float v_tiling_factor;
		
layout(location = 3) out flat int v_tex_id;
layout(location = 4) out flat int v_if_tex_grey;
layout(location = 5) out flat int v_is_font;
layout(location = 6) out flat int v_entity_id; // EDITOR ONLY

void main()
{
	v_color    = a_color;
	v_uv_coord = a_uv_coord;
	v_tiling_factor = a_tiling_factor;

	v_tex_id      = a_tex_id;
	v_if_tex_grey = a_if_tex_grey;
	v_is_font	  = a_is_font;
	v_entity_id	  = a_entity_id; // EDITOR ONLY

	gl_Position = u_camera.view_proj * vec4(a_position, 1.0);
}
