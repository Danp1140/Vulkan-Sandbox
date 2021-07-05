#version 410 core

in vec2 uvthru;

uniform sampler2DArray texturetoshoot;

layout(location=0) out vec4 color;

void main() {
    color=vec4(1, 1, 1, 0)*texture(texturetoshoot, vec3(uvthru, 0)).r+vec4(0, 0, 0, 1);
//    color=texture(texturetoshoot, uvthru).r*vec4(1, 1, 1, 1)+vec4(0, 0, 0, 1);
//    color=texture(texturetoshoot, uvthru).r*vec4(1, 1, 1, 1)+vec4(0, 0, 0, 1);
//    color=vec4(1, gl_FragCoord.x/1440.0f, 1, 1);
}
