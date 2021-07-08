#version 460 core

layout(location=0) in vec3 vertexposition;
layout(location=1) in vec3 vertexnormal;
layout(location=2) in vec2 vertexuv;
layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices, modelmatrix;
} constants;

layout(location=0) out vec3 positionthru;
layout(location=1) out vec3 normalthru;

void main() {
//    gl_Position=vec4(0.1f*vertexposition.x,-0.1f*vertexposition.y, 0.0f, 1.0f);
    gl_Position=constants.cameravpmatrices*constants.modelmatrix*vec4(vertexposition, 1.0f);
    positionthru=vertexposition;
    normalthru=vertexnormal;
}
