#version 450 core

layout(location = 0) out vec2 outUV;

const vec2 vertex[3] = vec2[3](
    vec2(-1., 1.),
    vec2(-1., -3.),
    vec2(3., 1.)
);

const vec2 uv[3] = vec2[3](
    vec2(0., 0.),
    vec2(0., 2.),
    vec2(2., 0.)
);

void main() {
//    outUV = uv[gl_VertexIndex];
//    gl_Position = vec4(vertex[gl_VertexIndex], 0.0f, 1.0f);
        outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
}