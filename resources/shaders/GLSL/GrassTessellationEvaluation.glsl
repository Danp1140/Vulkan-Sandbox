#version 460 core

layout(triangles, equal_spacing, cw) in;

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
}constants;

void main(){
    vec4 vertexposition=gl_in[0].gl_Position*gl_TessCoord.x
                        +gl_in[1].gl_Position*gl_TessCoord.y
                        +gl_in[2].gl_Position*gl_TessCoord.z;
    gl_Position=constants.cameravpmatrices*vertexposition;
}