#version 450

layout(location = 0) out vec2 v_uv_coord;

const vec3 positions[4] = vec3[]
(
    vec3(-1.0, -1.0,  0.0),
    vec3( 1.0, -1.0,  0.0),
    vec3(-1.0,  1.0,  0.0),
    vec3( 1.0,  1.0,  0.0)
);

const vec2 uvs[4] = vec2[]
(
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    v_uv_coord = uvs[gl_VertexIndex];
}
