#version 460 core

layout(location=0) in vec3 vertexposition;
layout(location=1) in vec3 vertexnormal;
layout(location=2) in vec2 vertexuv;

layout(location=1) out vec3 normalthru;
layout(location=2) out vec2 uvthru;

void main() {
    gl_Position=vec4(vertexposition, 1.0);
    uvthru=vertexuv;
    normalthru=vertexnormal;
}
