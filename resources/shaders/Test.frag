#version 460 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

//layout(binding=0) uniform sampler2D texturesampler;
layout(binding=0) uniform LightUniformBuffer{
    vec4 colortemp;
} lightuniformbuffer;

layout(location=0) out vec4 color;

void main() {
    vec3 lightposition=vec3(5.0f, 5.0f, 5.0f);
//    color=0.1f*vec4(1.0f, 1.0f, 1.0f, 0.0f)*dot(lightposition-position, normal)+vec4(0.1f, 0.1f, 0.1f, 1.0f);
//    color=lightuniformbuffer.colortemp*dot(lightposition-position, normal)+vec4(0.1f, 0.1f, 0.1f, 1.0f);
    color=lightuniformbuffer.colortemp;
//    color=vec4(uv.x, 0.0f, uv.y, 1.0f);
//    color=texture(texturesampler, uv);
}
