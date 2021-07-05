#version 410 core

in vec2 uvthru;

uniform sampler2D glyphtexture;

out vec4 color;

void main() {
    color=vec4(1, 0, 0, texture(glyphtexture, uvthru).r);
//    color=texture(glyphtexture, uvthru);
//    color=vec4(1, 0, 0, 1);
}
