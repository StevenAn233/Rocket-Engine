#version 450

#include "../include/binding_points.glsl"

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv_coord;

layout(binding = Sampler2D_0) uniform sampler2D u_texture;

layout(std140, binding = UBO_PostProcess) uniform Uniforms
{
    vec2 inverse_screen_size;
} u_data;

// given by Gemini 3.0 Pro preview

#define FXAA_SPAN_MAX    8.0
#define FXAA_REDUCE_MUL (1.0 / 8.0  )
#define FXAA_REDUCE_MIN (1.0 / 128.0)

float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114)); // trun RGB to Luma
}

void main()
{
    vec2 rcp_frame = u_data.inverse_screen_size;

    vec3 rgbNW = texture(u_texture, v_uv_coord + (vec2(-1.0, -1.0) * rcp_frame)).rgb;
    vec3 rgbNE = texture(u_texture, v_uv_coord + (vec2( 1.0, -1.0) * rcp_frame)).rgb;
    vec3 rgbSW = texture(u_texture, v_uv_coord + (vec2(-1.0,  1.0) * rcp_frame)).rgb;
    vec3 rgbSE = texture(u_texture, v_uv_coord + (vec2( 1.0,  1.0) * rcp_frame)).rgb;
    vec3 rgbM  = texture(u_texture, v_uv_coord).rgb;

    float lumaNW = rgb2luma(rgbNW);
    float lumaNE = rgb2luma(rgbNE);
    float lumaSW = rgb2luma(rgbSW);
    float lumaSE = rgb2luma(rgbSE);
    float lumaM  = rgb2luma(rgbM);

    float luma_min = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float luma_max = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float range = luma_max - luma_min;

    if(range < max(FXAA_REDUCE_MIN, luma_max * FXAA_REDUCE_MUL))
    {
        o_color = vec4(rgbM, 1.0);
        return;
    }

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dir_reduce = max
    (
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN
    );

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dir_reduce);
    
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * rcp_frame;

    vec3 rgbA = (1.0/2.0) * (
        texture(u_texture, v_uv_coord.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(u_texture, v_uv_coord.xy + dir * (2.0/3.0 - 0.5)).xyz);

    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texture(u_texture, v_uv_coord.xy + dir * (0.0/3.0 - 0.5)).xyz +
        texture(u_texture, v_uv_coord.xy + dir * (3.0/3.0 - 0.5)).xyz);

    float lumaB = rgb2luma(rgbB);

    if((lumaB < luma_min) || (lumaB > luma_max)) {
        o_color = vec4(rgbA, 1.0);
    } else {
        o_color = vec4(rgbB, 1.0);
    }
}
