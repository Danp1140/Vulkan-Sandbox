#version 460 core
//#extension GL_EXT_debug_printf:enable

#define MAX_LIGHTS 8

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices, modelmatrix;
    vec2 standinguv;
    uint numlights;
} constants;
layout(set=0, binding=0) uniform LightUniformBuffer{
    vec3 position;
    float intensity;
    vec3 forward;
    int lighttype;
    vec4 color;
}lightuniformbuffer[MAX_LIGHTS];
layout(set=1, binding=0) uniform sampler2D diffusesampler;
layout(set=1, binding=1) uniform sampler2D normalsampler;

layout(location=0) out vec4 color;

void main() {
    vec4 texcol=vec4(1.0f, 1.0f, 1.0f, 1.0f);
//    if(distance(constants.standinguv, uv)<0.05f) color*=vec4(0.0f, 1.0f, 0.0f, 1.0f);
//    if(uv.x>constants.standinguv.x-0.05f&&uv.x<constants.standinguv.x+0.05f
//        &&uv.y>constants.standinguv.y-0.05f&&uv.y<constants.standinguv.y+0.05f){
//        texcol=texture(texturesampler,
//            (constants.standinguv-uv)/0.1f+vec2(0.5f, 0.5f));
//    }
    texcol=texture(diffusesampler, uv*100.0f);
    vec3 texnorm=texture(normalsampler, uv).xyz;
    for(uint x=0;x<MAX_LIGHTS;x++){
        if (x<constants.numlights){
            if (lightuniformbuffer[x].lighttype==LIGHT_TYPE_SUN){
                vec3 lighttofrag=lightuniformbuffer[x].position-position;
                color+=texcol
//                color+=vec4(1.0f, 1.0f, 1.0f, 1.0f)
                *(lightuniformbuffer[x].color
                //shouldn't we be calculating this with a uniform vector from sun to origin, as suns are basically orthogonal?
//                *max(dot(normalize(lighttofrag), normal), 0)
                *max(dot(normalize(lightuniformbuffer[x].position), normal+texnorm), 0)
                *lightuniformbuffer[x].intensity
                +vec4(0.3f, 0.3f, 0.3f, 1.0f));
            }
            else {
                vec3 lighttofrag=lightuniformbuffer[x].position-position;
                color+=texcol
                *(lightuniformbuffer[x].color
                *max(dot(normalize(lighttofrag), normal+texnorm), 0)
                *lightuniformbuffer[x].intensity/pow(length(lighttofrag), 2)
                +vec4(0.1f, 0.1f, 0.1f, 1.0f));
            }
        }
        else break;
    }
    color.a=1.0;
}
