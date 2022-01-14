#version 460 core

layout(location=0) in vec3 vertexposition;
layout(location=1) in vec3 shutthefuckupvalidationlayers;

layout(std140, set=1, binding=0) readonly buffer ParticleStorageBuffer{
    mat4 modelmatrix[];
}psb;

void main(){
    gl_Position=psb.modelmatrix[gl_InstanceIndex]*vec4(vertexposition, 1.0);
}