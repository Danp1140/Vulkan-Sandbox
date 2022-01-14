#version 460 core

layout(location=1) in vec3 vertexnormals[];
layout(location=2) in vec2 vertexuvs[];

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 cameraposition;
    int numlights;
}constants;

layout(vertices=3) out;
layout(location=1) out vec3 normalsthru[];
layout(location=2) out vec2 uvsthru[];

void main() {
    //another potential goal would be scaling tessellation based off of relative edge length
    //scale by screen-space?
//    uint tesslevel=max(int(10.0/length(constants.cameraposition-(gl_in[0].gl_Position.xyz+gl_in[1].gl_Position.xyz+gl_in[2].gl_Position.xyz)/3)), 2);
    if(gl_InvocationID==0){
        gl_TessLevelOuter[2]=max(int(10.0/length(constants.cameraposition-(gl_in[0].gl_Position.xyz+gl_in[1].gl_Position.xyz)/2)), 1);
        gl_TessLevelOuter[0]=max(int(10.0/length(constants.cameraposition-(gl_in[1].gl_Position.xyz+gl_in[2].gl_Position.xyz)/2)), 1);
        gl_TessLevelOuter[1]=max(int(10.0/length(constants.cameraposition-(gl_in[2].gl_Position.xyz+gl_in[0].gl_Position.xyz)/2)), 1);
        gl_TessLevelInner[0]=max(int(10.0/length(constants.cameraposition-(gl_in[0].gl_Position.xyz+gl_in[1].gl_Position.xyz+gl_in[2].gl_Position.xyz)/3)), 2);
    }
    gl_out[gl_InvocationID].gl_Position=gl_in[gl_InvocationID].gl_Position;
    normalsthru[gl_InvocationID]=vertexnormals[gl_InvocationID];
    uvsthru[gl_InvocationID]=vertexuvs[gl_InvocationID];
}
