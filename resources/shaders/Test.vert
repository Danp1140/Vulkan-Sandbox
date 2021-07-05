#version 460 core

//layout(location=0) out vec3 colorthru;
layout(location=0) out vec4 colorout;

void main() {
    vec2 positions[3]={
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),x
    vec2(-0.5, 0.5)
    };

    vec4 colors[3]={
    vec4(0.0f, 1.0f, 0.0f, 0.0f),
    vec4(1.0f, 0.0f, 0.0f, 0.0f),
    vec4(0.0f, 0.0f, 1.0f, 0.0f)
    };

    gl_Position=vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
//    colorout=colors[gl_VertexIndex];
    switch(gl_VertexIndex){
        case(0):
            colorout=vec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;
        case(1):
            colorout=vec4(0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case(2):
            colorout=vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }
}
