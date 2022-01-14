#version 460 core

#define MAX_LIGHTS 2

layout(location=0) in vec3 vertexposition;
layout(location=1) in vec3 vertexnormal;
layout(location=2) in vec2 vertexuv;

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 camerapos;
    vec2 standinguv;
    uint numlights;
} constants;
layout(set=1, binding=0) uniform MeshUniformBuffer{
    mat4 modelmatrix,
        diffuseuvmatrix,
        normaluvmatrix,
        heightuvmatrix;
} mub;

layout(location=1) out vec3 normalthru;
layout(location=2) out vec2 diffuseuvthru;
layout(location=3) out vec2 normaluvthru;
layout(location=4) out vec2 heightuvthru;

vec2 uvTransform(mat4 m, vec2 uv){
    return vec2(dot(m[0].xyz, vec3(uv, 1.)-vec3(0.5, 0.5, 0.)),
                dot(m[1].xyz, vec3(uv, 1.)-vec3(0.5, 0.5, 0.)))
                +vec2(0.5, 0.5);
}

void main() {
    gl_Position=mub.modelmatrix*vec4(vertexposition, 1.);
//    gl_Position=vec4(vertexposition, 1.);
    normalthru=vertexnormal;
    vec3 tempuv=vec3(vertexuv, 1.), finaluvtemp;
    diffuseuvthru=uvTransform(mub.diffuseuvmatrix, vertexuv);
    normaluvthru=uvTransform(mub.normaluvmatrix, vertexuv);
    heightuvthru=uvTransform(mub.heightuvmatrix, vertexuv);
}
