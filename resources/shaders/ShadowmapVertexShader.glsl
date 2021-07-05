#version 410 core

layout(location=0) in vec3 vertexposition;
//consider receiving and using vertex normal???

uniform mat4 lightvpmatrices, modelmatrix;

void main() {
    gl_Position=lightvpmatrices*modelmatrix*vec4(vertexposition, 1);
}
