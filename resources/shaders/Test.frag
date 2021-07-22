#version 460 core
//#extension GL_EXT_debug_printf:enable

#define LIGHT_TYPE_SUN   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_PLANE 2
#define LIGHT_TYPE_SPOT  3

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

layout(set=0, binding=0) uniform LightUniformBuffer{
    vec3 position;
    float intensity;
    vec3 forward;
    int lighttype;
    vec4 color;
} lightuniformbuffer;
layout(set=1, binding=0) uniform sampler2D texturesampler;

layout(location=0) out vec4 color;

void main() {
    if(lightuniformbuffer.lighttype==LIGHT_TYPE_SUN){

    }
    else{
        vec3 lighttofrag=lightuniformbuffer.position-position;
        color=texture(texturesampler, uv*10.0f)
                /**lightuniformbuffer.color*/
                *max(dot(lighttofrag, normal), 0)
                *lightuniformbuffer.intensity/pow(length(lighttofrag), 2);
//        color=texture(texturesampler, uv*10.0f)*1000.0f;
    }
}
