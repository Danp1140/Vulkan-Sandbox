#version 460 core

layout(location=0) in vec2 uv;

layout(set=0, binding=0) uniform sampler2D texturesampler;

layout(location=0) out vec4 color;

void main() {
    // below is clamp to cope with new float buffer, should figure out how to appropriately scale alpha for better font aliasing
    color=vec4(1.0f, 1.0f, 1.0f, clamp(texture(texturesampler, uv).r, 0., 1.));
//    color=vec4(1.0f, 1.0f, 1.0f, 1.);
//    color=texture(texturesampler, uv);
//    color=vec4(1.0f, 1.0f, 1.0f, 1.0f);
//    color=vec4(uv.x, uv.y, 0.0f, 1.0f);
//    color=vec4(1, 0, 0, 1);
}
