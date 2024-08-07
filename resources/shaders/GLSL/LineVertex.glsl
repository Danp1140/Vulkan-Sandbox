#version 460 core

layout(location = 0) in vec3 vertexposition;

layout(push_constant) uniform PushConstants{
    mat4 vp;
} constants;

void main() {
    gl_Position = constants.vp * vec4(vertexposition, 1);
}
