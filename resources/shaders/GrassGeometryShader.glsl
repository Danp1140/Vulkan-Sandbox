#version 410 core

layout(points) in;
layout(line_strip, max_vertices=2) out;

uniform mat4 cameravpmatrices;

//hey fun fact we could sample textures here, meaning we could have like weight/param painting implemented here if thats
//efficient...

void main() {
    gl_Position=cameravpmatrices*gl_in[0].gl_Position;
    EmitVertex();
    gl_Position=cameravpmatrices*gl_in[0].gl_Position+cameravpmatrices*vec4(0, 5, 0, 0);
    EmitVertex();
    EndPrimitive();
}
