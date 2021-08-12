#version 460 core

layout(push_constant) uniform PushConstants{
    vec3 forward, dx, dy, sunposition;
} pushconstants;

layout(location=0) out vec4 color;

void main() {
    //unsure of the true scaling factors on this, should take some time to logic it out eventually
    vec3 look=pushconstants.forward
              +pushconstants.dx*(float(gl_FragCoord.x)-1440.0)
              +pushconstants.dy*(float(gl_FragCoord.y)-900.0);

    vec3 nlook=normalize(look);

    //should pass an aspect ratio/horivert reses...
    //could be a specialization constant
    if(length(nlook-normalize(pushconstants.sunposition))<0.05){
        color=vec4(1.0);
        return;
    }

    color=vec4((-nlook.y+1.0f)/2.0f, (-nlook.y+1.0f)/2.0f, 0.9f, 1.0f);
}
