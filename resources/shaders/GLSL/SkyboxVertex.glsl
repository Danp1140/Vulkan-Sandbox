#version 460 core

const vec2 vertexpositions[4]={
    vec2(-1.0f, -1.0f),
    vec2(-1.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, -1.0f)
};
const uint vertexindices[6]={
    0, 1, 2,
    2, 3, 0
};

void main() {
    gl_Position=vec4(vertexpositions[vertexindices[gl_VertexIndex]], 0.99999f, 1.0f);
}
