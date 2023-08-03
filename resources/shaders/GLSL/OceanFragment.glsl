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

//may replace the normal here with the halfway dir (i.e. theta is determined by dotting v and h)
float schlickApprox(float costheta, float n1, float n2){
    float r0=pow((n1-n2)/(n1+n2), 2.);
    return r0+(1.-r0)*pow(1.-costheta, 5.);
}

void main() {
    vec3 normaldir=normalize(normal+0.5*texture(normalsampler, 20. * uv).xyz);
//    vec3 halfwaydir=normalize(lightuniformbuffer[0].position-position+constants.cameraposition-position);
    vec3 halfwaydir=normalize(lightuniformbuffer[0].position+constants.cameraposition-position);
    vec4 specular=lightuniformbuffer[0].color*pow(max(dot(normaldir, halfwaydir), 0.), PHONG_EXPONENT);

    float r0=pow((1.-1.33)/(1.+1.33), 2.);
    float fresnel=r0+(1.-r0)*pow(1.-dot(-normalize(constants.cameraposition-position), normaldir), 5.); // TODO: use schlickApprox function which we have created

//    color=vec4(0.3, 0.3, 0.7, 1.0)*max(dot(normalize(lightuniformbuffer[0].position), normaldir), 0.)+specular;       //could scroll uvs w/ time (& rotate w/ wind) (combine into single matrix to send)
    // we should technically be normalizing input comps
    vec3 ref=normalize(reflect(-constants.cameraposition+position, normaldir));
    vec4 reflectsample = texture(environmentsampler, ref);
    vec4 screenspacereftemp;
    vec3 ssref;
    float maxdist = (50. - constants.cameravpmatrices[2][3] - constants.cameravpmatrices[2][0] * position.x - constants.cameravpmatrices[2][1] * position.y - constants.cameravpmatrices[2][2] * position.z)
    / (constants.cameravpmatrices[2][0] * ref.x + constants.cameravpmatrices[2][1] * ref.y + constants.cameravpmatrices[2][2] * ref.z);
    // i technically don't have any verification that maxdist works correctly here either
    for (float i = 0.1; i < 50.; i += 0.1) {
        screenspacereftemp = constants.cameravpmatrices * vec4(position + ref * i, 1.); // this op could probably be made more efficient w/o doing entire matrix op
        ssref = screenspacereftemp.xyz / screenspacereftemp.w;
        ssref += vec3(1., 1., 0.); // convert to uv space
        ssref /= vec3(2., 2., 1.);
        if (ssref.x > 1.f || ssref.x < 0.f || ssref.y > 1.f || ssref.y < 0.f || ssref.z < 0.f) {
            break;
        }
        if (texture(ssrrdepthsampler, ssref.xy).r < ssref.z) {
            reflectsample = texture(ssrrcolorsampler, ssref.xy);
            break;
        }
    }
    // if this structure works, implement a binary search after

    // TODO: move this to rayMarch func
    // TODO: implement binary search & parameterization for more precise sampling
    // TODO: implement blending when samples are not available; perhaps integrate with more advanced prediction of offscreen rays

    vec3 refraction = normalize(refract(normalize(-constants.cameraposition+position), normalize(normaldir), 1. / 1.33));
    maxdist = (50. - constants.cameravpmatrices[2][3] - constants.cameravpmatrices[2][0] * position.x - constants.cameravpmatrices[2][1] * position.y - constants.cameravpmatrices[2][2] * position.z)
    / (constants.cameravpmatrices[2][0] * refraction.x + constants.cameravpmatrices[2][1] * refraction.y + constants.cameravpmatrices[2][2] * refraction.z);
    vec4 refractsample = vec4(-1.);
    refractsample = vec4(1., 0., 0., 1.);
//     maxdist is being mis-calculated here
    for (float i = 0.1; i < 50.; i += 0.1) {
        screenspacereftemp = constants.cameravpmatrices * vec4(position + refraction * i, 1.); // this op could probably be made more efficient w/o doing entire matrix op
        ssref = screenspacereftemp.xyz / screenspacereftemp.w;
        ssref += vec3(1., 1., 0.); // convert to uv space
        ssref /= vec3(2., 2., 1.);
        if (ssref.x > 1.f || ssref.x < 0.f || ssref.y > 1.f || ssref.y < 0.f || ssref.z < 0.f) {
            refractsample = vec4(-1.);
            break;
        }
        if (texture(ssrrdepthsampler, ssref.xy).r < ssref.z) {
            refractsample = texture(ssrrcolorsampler, ssref.xy);
            break;
        }
    }

    if (refractsample.x == -1.) {
        color = fresnel * reflectsample;
        color.a = 1. / fresnel;
    }
    else {
        color = mix(refractsample, reflectsample, 1. / fresnel);
        color.a = 1.;
    }
}
