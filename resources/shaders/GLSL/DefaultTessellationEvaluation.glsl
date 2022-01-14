#version 460 core

#define DUV 0.01
#define MAX_LIGHTS 2

layout(triangles, equal_spacing, cw) in;
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
layout(set=0, binding=0) uniform LightUniformBuffer{
    mat4 vpmatrices;
    mat4 lspmatrix;
    vec3 position;
    float intensity;
    vec3 forward;
    int lighttype;
    vec4 color;
}lub[MAX_LIGHTS];
layout(set=1, binding=3) uniform sampler2D heightsampler;


layout(location=0) out vec3 positionthru;
layout(location=1) out vec3 normalthru;
layout(location=2) out vec3 computednorm;
layout(location=3) out vec2 diffuseuvsthru;
layout(location=4) out vec2 normaluvsthru;
layout(location=5) out vec3 shadowpositionthru[MAX_LIGHTS];

float sampleHeight(vec2 uv){
    return texture(heightsampler, uv).r;
}

vec4 quadInterp(vec4 a, vec4 b, vec4 c, vec4 d){
    return mix(
            mix(a, b, gl_TessCoord.x),
            mix(c, d, gl_TessCoord.x),
            gl_TessCoord.y);
}
vec3 quadInterp(vec3 a, vec3 b, vec3 c, vec3 d){
    return mix(
            mix(a, b, gl_TessCoord.x),
            mix(c, d, gl_TessCoord.x),
            gl_TessCoord.y);
}
vec2 quadInterp(vec2 a, vec2 b, vec2 c, vec2 d){
    return mix(
            mix(a, b, gl_TessCoord.x),
            mix(c, d, gl_TessCoord.x),
            gl_TessCoord.y);
}

vec2 triInterp(vec2 a, vec2 b, vec2 c){
    return a*gl_TessCoord.x+b*gl_TessCoord.y+c*gl_TessCoord.z;
}
vec3 triInterp(vec3 a, vec3 b, vec3 c){
    return a*gl_TessCoord.x+b*gl_TessCoord.y+c*gl_TessCoord.z;
}
vec4 triInterp(vec4 a, vec4 b, vec4 c){
    return a*gl_TessCoord.x+b*gl_TessCoord.y+c*gl_TessCoord.z;
}

void main(){
    vec4 vertexposition=triInterp(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
    normalthru=triInterp(vertexnormals[0], vertexnormals[1], vertexnormals[2]);
    diffuseuvsthru=triInterp(diffusevertexuvs[0], diffusevertexuvs[1], diffusevertexuvs[2]);
    normaluvsthru=triInterp(normalvertexuvs[0], normalvertexuvs[1], normalvertexuvs[2]);
    vec2 heightuvsthru=triInterp(heightvertexuvs[0], heightvertexuvs[1], heightvertexuvs[2]);

    vertexposition.y+=sampleHeight(heightuvsthru);
    gl_Position=constants.cameravpmatrices*vertexposition;
//    vec4 pretemp=lub[0].lspmatrix*vertexposition;
//    pretemp/=pretemp.w;
//    gl_Position=lub[0].vpmatrices*pretemp;
    positionthru=vertexposition.xyz;
    computednorm=cross(vec3(0.0, sampleHeight(heightuvsthru+vec2(0.0, DUV)), 1.0), vec3(1.0, sampleHeight(heightuvsthru+vec2(DUV, 0.0)), 0.0))+
                            cross(vec3(0.0, sampleHeight(heightuvsthru+vec2(0.0, -DUV)), -1.0), vec3(-1.0, sampleHeight(heightuvsthru+vec2(-DUV, 0.0)), 0.0));
    vec4 temp;
    for(uint x=0;x<MAX_LIGHTS;x++){
        if(x<constants.numlights){
            temp=lub[x].lspmatrix*vertexposition;
//            shadowpositionthru[x]=(
//            mat4(0.5, 0., 0., 0.,
//                0., 0.5, 0., 0.,
//                0., 0., 1., 0.,
//                0.5, 0.5, 0., 1.)*lub[x].vpmatrices*(temp/temp.w)).xyz;
            temp=mat4(0.5, 0., 0., 0.,
                         0., 0.5, 0., 0.,
                         0., 0., 1., 0.,
                         0.5, 0.5, 0., 1.)*lub[x].vpmatrices*(temp/temp.w);
             shadowpositionthru[x]=(temp/temp.w).xyz;
        }
        else break;
    }
}