#version 460 core
//#extension GL_EXT_debug_printf:enable

#define MAX_LIGHTS 2

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

#define PHONG_EXPONENT 100.

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 computednorm;
layout(location=3) in vec2 diffuseuv;
layout(location=4) in vec2 normaluv;
layout(location=5) in vec3 shadowposition[MAX_LIGHTS];

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
layout(set=0, binding=1) uniform samplerCube environmentsampler;
layout(set=1, binding=1) uniform sampler2D diffusesampler;
layout(set=1, binding=2) uniform sampler2D normalsampler;
layout(set=1, binding=3) uniform sampler2D heightsampler;
layout(set=1, binding=4) uniform sampler2DShadow shadowsampler[MAX_LIGHTS];

layout(location=0) out vec4 color;

void main() {
    const vec4 ambient=vec4(0.1, 0.1, 0.1, 1.);
    vec4 diffuse=texture(diffusesampler, diffuseuv);
    vec3 normaldir=normalize(normal+computednorm+texture(normalsampler, normaluv).xyz);
    vec3 lightdir, halfwaydir;
    float lambertian, specular;
    //alright so it kinda seems like the biggest problem i have is in my view vector my my specularity. something about
    //it is fully incorrect...
    //what other people use is actually a single cam-space view vector, different from our varying world-space view vector
    for(uint x=0;x<MAX_LIGHTS;x++){
        if (x<constants.numlights){
            //perhaps just fiddle with bias here a bit more?
            float shadow=texture(shadowsampler[x], shadowposition[x]+vec3(0., 0., clamp(0.005*tan(acos(dot(normaldir, vec3(1.)))), 0., 0.01)));
//            if(shadow==0) continue;   //could we do an early escape just adding diffuse*ambient??? (ambient is kinda non-directional => no lambertian???)
            lub[x].lighttype==LIGHT_TYPE_SUN?lightdir=normalize(lub[x].position):lightdir=normalize(lub[x].position-position);
            halfwaydir=normalize(lightdir+normalize(constants.camerapos-position));
            specular=pow(max(dot(normaldir, halfwaydir), 0.), PHONG_EXPONENT);
            lambertian=max(dot(lightdir, normaldir), 0)*lub[x].intensity;
            if(lub[x].lighttype!=LIGHT_TYPE_SUN) lambertian/=pow(length(lub[x].position-position), 2);
            color+=lub[x].color*(diffuse*lambertian*shadow+diffuse*ambient);
        }
        else break;
    }
    color.a=1.;
}
