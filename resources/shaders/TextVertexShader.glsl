#version 410 core

layout(location=0) in vec2 vertexposition;
layout(location=1) in vec2 vertexuv;

out vec2 uvthru;

void main() {
    gl_Position=vec4(vertexposition, 0.0f, 1.0f);
    uvthru=vertexuv;
}
