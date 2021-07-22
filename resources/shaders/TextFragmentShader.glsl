#version 460 core

layout(location=0) in vec2 uv;

layout(set=0, binding=0) uniform sampler2D texturesampler;

layout(location=0) out vec4 color;

void main() {
//    color=vec4(1, 1, 1, texture(texturesampler, uv).r);
    color=texture(texturesampler, uv);
//    color=vec4(uv.x, uv.y, 0.0f, 1.0f);
//    color=vec4(1, 0, 0, 1);
}
