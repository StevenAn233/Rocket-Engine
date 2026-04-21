#version 450

#include "../include/binding_points.glsl"

layout(location = 0) out vec4 o_color;
layout(location = 1) out int  o_entity_id; // EDITOR ONLY

layout(location = 0) in vec4  v_color;
layout(location = 1) in vec2  v_uv_coord;
layout(location = 2) in float v_tiling_factor;

layout(location = 3) in flat int v_tex_id;
layout(location = 4) in flat int v_if_tex_grey;
layout(location = 5) in flat int v_is_font;
layout(location = 6) in flat int v_entity_id; // EDITOR ONLY

layout(binding = Sampler2D_0) uniform sampler2D u_textures[32];

void main()
{
	o_entity_id = v_entity_id; // EDITOR ONLY

	vec4 tex_color;
	if(bool(v_is_font)) {
		tex_color = vec4(1.0, 1.0, 1.0, texture(u_textures[v_tex_id], v_uv_coord).r);
	} else {
		tex_color = texture(u_textures[v_tex_id], v_uv_coord * v_tiling_factor);
	}

//	const float alpha_threshold = 0.05; // hard-coded
//  if(tex_color.a < alpha_threshold) { discard; }

	if(!bool(v_if_tex_grey))
	{
		vec4 final_color = tex_color * v_color;
		final_color.rgb *= final_color.a;
		o_color = final_color;
		return;
	}
	vec3 make_gray_weights = vec3(0.2126, 0.7152, 0.0722);
	float luminance  = dot(tex_color.rgb, make_gray_weights);
	vec4 final_color = vec4(vec3(luminance), tex_color.a) * v_color;
	final_color.rgb *= final_color.a;
	o_color = final_color;
}
