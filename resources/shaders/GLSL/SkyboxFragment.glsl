#version 460 core

layout(location=0) in vec3 fragpos;

layout(push_constant) uniform PushConstants{
    mat4 cameravpmatrices;
    vec3 sunposition;
} pushconstants;
layout(binding=0, set=0) uniform samplerCube skybox;

layout(location=0) out vec4 color;

void main() {
// this entire skybox system is due for a rewrite, both to make it look better/more realistic
// and to make it maximally efficient


//    color=vec4((-nlook.y+1.0f)/2.0f, (-nlook.y+1.0f)/2.0f, 0.9f, 1.0f);
//    color=vec4((-normalize(fragpos).y+1.)/2., (-normalize(fragpos).y+1.)/2., 0.9, 1.);
    color=texture(skybox, fragpos);
//      if(dot(fragpos, pushconstants.sunposition)/(length(fragpos)*length(pushconstants.sunposition))<0.05){
    if(length(normalize(fragpos)-normalize(pushconstants.sunposition))<0.05){
	// TODO: use actual sun intensity; will vary with angle
        color=vec4(100.);
    }
}
