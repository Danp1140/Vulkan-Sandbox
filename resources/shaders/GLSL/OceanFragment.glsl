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
layout(set=1, binding=2) uniform sampler2D ssrrcolorsampler;
layout(set=1, binding=3) uniform sampler2D ssrrdepthsampler;

layout(location=0) out vec4 color;

// clipdist param is naiive impl, fix later
void rayMarch(inout vec3 r, in float clipdist, in float marchstep) {
    vec4 ssr;
    // TODO: don't bother checking a clipdist, check ssr.z < 1. (and other bounds)
    for (float i = marchstep; i < clipdist; i += marchstep) {
        ssr = constants.cameravpmatrices * vec4(position + r * i, 1.);
        ssr.xyz /= ssr.w;
        ssr += vec4(1., 1., 0., 0.);
        ssr /= vec4(2., 2., 1., 1.);
        // TODO: refine this boundary procedure
        // is -1. the right OOB #?
        if (ssr.x > 1. || ssr.x < 0. || ssr.y > 1. || ssr.y < 0.) {
            r = vec3(-1.);
            return;
        }
        if (texture(ssrrdepthsampler, ssr.xy).r < ssr.z) {
            r = ssr.xyz;
            return;
        }
    }
    r = vec3(-1.);
}

void sampleSSRR(out vec4 combinedsample, in float eta, in vec3 n) {
    vec3 view = normalize(position - constants.cameraposition);
    vec3 reflectray = reflect(view, n),
    refractray = refract(view, n, eta);
    rayMarch(reflectray, 50., 0.1);
    rayMarch(refractray, 50., 0.1);
    float r0 = pow((1. - 1.33) / (1. + 1.33), 2.);
    float fresnel = r0 + (1. - r0) * pow(1. - dot(view, n), 5.);
    vec4 reflectcolor = texture(environmentsampler, reflectray), refractcolor;
    if (reflectray.x != -1.) reflectcolor = texture(ssrrcolorsampler, reflectray.xy);
    if (refractray.x != -1.) refractcolor = texture(ssrrcolorsampler, refractray.xy);
    // TODO: combine this w/ above logic
    if (refractray.x == -1.) {
        combinedsample = fresnel * reflectcolor;
        combinedsample.a = 1. / fresnel;
    }
    else {
        combinedsample = mix(refractcolor, reflectcolor, 1. / fresnel);
        combinedsample.a = 1.;
    }

}

void main() {
    vec3 normaldir=normalize(normal+0.5*texture(normalsampler, 20. * uv).xyz);
//    vec3 halfwaydir=normalize(lightuniformbuffer[0].position-position+constants.cameraposition-position);
    vec3 halfwaydir=normalize(lightuniformbuffer[0].position+constants.cameraposition-position);
    vec4 specular=lightuniformbuffer[0].color*pow(max(dot(normaldir, halfwaydir), 0.), PHONG_EXPONENT);



//    color=vec4(0.3, 0.3, 0.7, 1.0)*max(dot(normalize(lightuniformbuffer[0].position), normaldir), 0.)+specular;       //could scroll uvs w/ time (& rotate w/ wind) (combine into single matrix to send)
//    // TODO: implement binary search & parameterization for more precise sampling
//    // TODO: implement blending when samples are not available; perhaps integrate with more advanced prediction of offscreen rays
    vec4 reflectsample, refractsample;
    sampleSSRR(color, 1. / 1.33, normaldir);
}
