#version 460 core

layout(location=0) in vec3 vertexposition;

layout(set=1, binding=0) uniform MeshUniformBuffer{
    mat4 modelmatrix,
        diffusematrix,
        normalmatrix,
        heightmatrix;
}mub;

layout(push_constant) uniform PushConstants{
    mat4 lightvpmatrices, lspmatrix;
} pc;

void main(){
//    vec4 temp = pc.lspmatrix * mub.modelmatrix * vec4(vertexposition, 1.f);    //pretty inefficient, see if we can recombine later
//    temp /= temp.w;
    // theres a chance that we don't need to do the w divide here, and can just let it happen in between shaders normally
    // see DefaultTessellationEvaluation.glsl for simlar change to be considered
//    gl_Position = pc.lightvpmatrices * temp;
    gl_Position = pc.lightvpmatrices * pc.lspmatrix * mub.modelmatrix * vec4(vertexposition, 1.f);
}