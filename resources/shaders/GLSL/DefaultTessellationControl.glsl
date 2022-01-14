#version 460 core

#define PIXELS_PER_EDGE 0.01
#define PIXELS_PER_FACE 0.01
#define MAX_LIGHTS 2

layout(location=1) in vec3 vertexnormals[];
layout(location=2) in vec2 diffusevertexuvs[];
layout(location=3) in vec2 normalvertexuvs[];
layout(location=4) in vec2 heightvertexuvs[];

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 camerapos;
    vec2 standinguv;
    uint numlights;
} constants;

layout(vertices=3) out;
layout(location=1) out vec3 normalsthru[];
layout(location=2) out vec2 diffuseuvsthru[];
layout(location=3) out vec2 normaluvsthru[];
layout(location=4) out vec2 heightuvsthru[];


const uint maxtesslevel=2;
void main(){
    if(gl_InvocationID==0){
        vec4 screenspacepos[3]={
            constants.cameravpmatrices*gl_in[0].gl_Position,
            constants.cameravpmatrices*gl_in[1].gl_Position,
            constants.cameravpmatrices*gl_in[2].gl_Position
        };
        float A=abs(distance(screenspacepos[1].xy, screenspacepos[2].xy)),
                B=abs(distance(screenspacepos[2].xy, screenspacepos[0].xy)),
                C=abs(distance(screenspacepos[0].xy, screenspacepos[1].xy));
        float p=(A+B+C)/2.;
        float R=sqrt(p*(p-A)*(p-B)*(p-C));
        gl_TessLevelOuter[0]=max(int(A/PIXELS_PER_EDGE), 1);
        gl_TessLevelOuter[1]=max(int(B/PIXELS_PER_EDGE), 1);
        gl_TessLevelOuter[2]=max(int(C/PIXELS_PER_EDGE), 1);
        gl_TessLevelInner[0]=max(int(R/PIXELS_PER_FACE), 1);
    }
////    if(gl_InvocationID==0){
////        gl_TessLevelOuter[2]=max(int(10.0/length(constants.camerapos-(gl_in[0].gl_Position.xyz+gl_in[1].gl_Position.xyz)/2)), 1);
////        gl_TessLevelOuter[0]=max(int(10.0/length(constants.camerapos-(gl_in[1].gl_Position.xyz+gl_in[2].gl_Position.xyz)/2)), 1);
////        gl_TessLevelOuter[1]=max(int(10.0/length(constants.camerapos-(gl_in[2].gl_Position.xyz+gl_in[0].gl_Position.xyz)/2)), 1);
////        gl_TessLevelInner[0]=max(int(10.0/length(constants.camerapos-(gl_in[0].gl_Position.xyz+gl_in[1].gl_Position.xyz+gl_in[2].gl_Position.xyz)/3)), 2);
////    }
    gl_out[gl_InvocationID].gl_Position=gl_in[gl_InvocationID].gl_Position;
    normalsthru[gl_InvocationID]=vertexnormals[gl_InvocationID];
    diffuseuvsthru[gl_InvocationID]=diffusevertexuvs[gl_InvocationID];
    normaluvsthru[gl_InvocationID]=normalvertexuvs[gl_InvocationID];
    heightuvsthru[gl_InvocationID]=heightvertexuvs[gl_InvocationID];
}