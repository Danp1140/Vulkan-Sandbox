#version 460 core

#define DUV 0.05

layout(triangles, equal_spacing, cw) in;
layout(location=1) in vec3 vertexnormals[];
layout(location=2) in vec2 vertexuvs[];

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 cameraposition;
    int numlights;
}constants;
layout(set=1, binding=0) uniform sampler2D heightsampler;

layout(location=0) out vec3 positionthru;
layout(location=1) out vec3 normalthru;
layout(location=2) out vec2 uvthru;

float calculateHeight(vec2 uv);

void main() {
    vec4 vertexposition=gl_in[0].gl_Position*gl_TessCoord.x
                        +gl_in[1].gl_Position*gl_TessCoord.y
                        +gl_in[2].gl_Position*gl_TessCoord.z;
    vec2 vertexuv=vertexuvs[0]*gl_TessCoord.x
                        +vertexuvs[1]*gl_TessCoord.y
                        +vertexuvs[2]*gl_TessCoord.z;

// commented out to troubleshoot ssrr, add back later
    vertexposition.y+=calculateHeight(vertexuv);
//    vertexposition.y += 1.5f;
    //do we really have to do this vp multiplication every time???
    gl_Position=constants.cameravpmatrices*vertexposition;
    uvthru=vertexuv;
    // this is a lot of computation lol, esp bad if we tess more
    vec3 computednormal=cross(vec3(0.0, calculateHeight(vertexuv+vec2(0.0, DUV)), 1.0), vec3(1.0, calculateHeight(vertexuv+vec2(DUV, 0.0)), 0.0))+
            cross(vec3(0.0, calculateHeight(vertexuv+vec2(0.0, -DUV)), -1.0), vec3(-1.0, calculateHeight(vertexuv+vec2(-DUV, 0.0)), 0.0));
    //    normalthru=vertexnormals[0]*gl_TessCoord.x
//                +vertexnormals[1]*gl_TessCoord.y
//                +vertexnormals[2]*gl_TessCoord.z
//                +5*computednormal;
    //same as above, add back in a sec
    normalthru=computednormal;
    normalthru = vertexnormals[0];
    positionthru=vertexposition.xyz;
}

float calculateHeight(vec2 uv){
//    return sin(uv.x*25.0)*0.25+cos(uv.y*25.0)*0.25;
    return texture(heightsampler, uv).r;
//    return float(gl_PrimitiveID)/
}