#version 410 core
#define MAX_LIGHTS 4

layout(location=0) in vec3 vertexposition;
layout(location=1) in vec3 vertexnormal;
layout(location=2) in vec2 vertexuv;

uniform mat4 cameravpmatrices, modelmatrix, lightvpmatrices[MAX_LIGHTS*2];
uniform uint numlights;

out vec3 positionthru;
out vec3 normalthru;
out vec2 uvthru;
out vec4 shadowpositionthru[MAX_LIGHTS];
flat out uint numlightsthru;

void main() {
    gl_Position=cameravpmatrices*modelmatrix*vec4(vertexposition, 1);
    positionthru=vec3(modelmatrix*vec4(vertexposition, 1));
    normalthru=vertexnormal;
    uvthru=vertexuv;
    for(int x=0;x<numlights;x++) shadowpositionthru[x]=lightvpmatrices[x]*modelmatrix*vec4(vertexposition, 1);
    numlightsthru=numlights;
}
