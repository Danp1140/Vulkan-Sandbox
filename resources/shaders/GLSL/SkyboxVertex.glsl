#version 460 core

//const vec2 vertexpositions[4]={
//    vec2(-1.0f, -1.0f),
//    vec2(-1.0f, 1.0f),
//    vec2(1.0f, 1.0f),
//    vec2(1.0f, -1.0f)
//};
//const uint vertexindices[6]={
//    0, 1, 2,
//    2, 3, 0
//};

const vec3 cubevertexpositions[8]={
    vec3(1., 1., 1.),
    vec3(-1., 1., 1.),
    vec3(1., -1., 1.),
    vec3(1., 1., -1.),
    vec3(-1., -1., 1.),
    vec3(-1., 1., -1.),
    vec3(1., -1., -1.),
    vec3(-1., -1., -1.)
};

const uint cubevertexindices[36]={
    0, 3, 6,
    0, 6, 2,
    1, 7, 5,
    1, 4, 7,
    0, 1, 3,
    3, 1, 5,
    2, 6, 4,
    4, 6, 7,
    0, 2, 4,
    0, 4, 1,
    5, 7, 6,
    5, 6, 3
};

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 sunposition;
} pushconstants;

layout(location=0) out vec3 posthru;

void main() {
    posthru=cubevertexpositions[cubevertexindices[gl_VertexIndex]];
    gl_Position=pushconstants.cameravpmatrices*vec4(cubevertexpositions[cubevertexindices[gl_VertexIndex]], 1.);
}
