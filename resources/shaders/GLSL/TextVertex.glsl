#version 460 core

layout(push_constant) uniform PushConstants{
    vec2 position, scale;
    float rotation;
} pushconstants;

layout(location=0) out vec2 uvout;

vec2 positions[4]={
    pushconstants.position,
    pushconstants.position+vec2(0, pushconstants.scale.y),
    pushconstants.position+pushconstants.scale,
    pushconstants.position+vec2(pushconstants.scale.x, 0),
};
const vec2 uvs[4]={
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1)
};
const uint indices[6]={
    0, 1, 2,
    2, 3, 0
};

void main() {
    gl_Position=vec4(positions[indices[gl_VertexIndex]], 0.0f, 1.0f);
    uvout=uvs[indices[gl_VertexIndex]];
}
