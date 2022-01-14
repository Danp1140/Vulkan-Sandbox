#version 460 core

#define MAX_LIGHTS 2

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

#define PHONG_EXPONENT 2

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;      //could we calc this in compute shader???
layout(location=2) in vec2 uv;

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 cameraposition;
    int numlights;
}constants;
layout(set=0, binding=0) uniform LightUniformBuffer{
    vec3 position;
    float intensity;
    vec3 forward;
    int lighttype;
    vec4 color;
}lightuniformbuffer[MAX_LIGHTS];
layout(set=0, binding=1) uniform samplerCube environmentsampler;
layout(set=1, binding=1) uniform sampler2D normalsampler;
layout(set=1, binding=2) uniform sampler2D screensampler;

layout(location=0) out vec4 color;

//may replace the normal here with the halfway dir (i.e. theta is determined by dotting v and h)
float schlickApprox(float costheta, float n1, float n2){
    float r0=pow((n1-n2)/(n1+n2), 2.);
    return r0+(1.-r0)*pow(1.-costheta, 5.);
}

void main() {
    vec3 normaldir=normalize(normal+0.5*texture(normalsampler, 20.*uv).xyz);
//    vec3 normaldir=normalize(normal);
//    vec3 normaldir=normalize(texture(normalsampler, uv).xyz);
//    vec3 halfwaydir=normalize(lightuniformbuffer[0].position-position+constants.cameraposition-position);
    vec3 halfwaydir=normalize(lightuniformbuffer[0].position+constants.cameraposition-position);
    vec4 specular=lightuniformbuffer[0].color*pow(max(dot(normaldir, halfwaydir), 0.), PHONG_EXPONENT);

//    float fresnel=schlickApprox(dot(-normalize(constants.cameraposition-position), normaldir), 1., 1.33);
    float r0=pow((1.-1.33)/(1.+1.33), 2.);
    float fresnel=r0+(1.-r0)*pow(1.-dot(-normalize(constants.cameraposition-position), normaldir), 5.);

//    color=vec4(0.3, 0.3, 0.7, 1.0)*max(dot(normalize(lightuniformbuffer[0].position), normaldir), 0.)+specular;       //could scroll uvs w/ time (& rotate w/ wind) (combine into single matrix to send)
    vec3 ref=reflect(-constants.cameraposition+position, normaldir);
    if(ref.y<0) ref.y*=-1.;     //this is a /bit/ of a bodge, think harder about how to handle reflections from the backface
            //above bodge also makes unrealiztic reflections
    //actually: rotating our normal map raises some important questions about the coordinate space of our normal maps


    color=fresnel*texture(environmentsampler, ref);
    color.a=1./fresnel;
    color=texture(screensampler, gl_FragCoord.xy);
}
