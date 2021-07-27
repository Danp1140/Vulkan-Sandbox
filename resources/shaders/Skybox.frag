#version 460 core

layout(push_constant) uniform PushConstants{
    vec3 forward, dx, dy, cameraposition;
} pushconstants;

layout(location=0) out vec4 color;

void main() {

//    vec3 look=pushconstants.cameraforward;
    //below conversion is a bit wrong lol
//    vec3 look=yrotation*xrotation*vec3(float(gl_FragCoord.x)/1440.0f-1.0f, -float(gl_FragCoord.y)/900.0f+1.0f, 1.0f/tan(pushconstants.fovy));

    //unsure of the true scaling factors on this, should take some time to logic it out eventually
    vec3 look=pushconstants.forward
              +pushconstants.dx*(float(gl_FragCoord.x)/1440.0f-1.0f)
              +pushconstants.dy*(float(gl_FragCoord.y)/900.0f-1.0f);

//    vec3 look=pushconstants.cameraposition-gl_FragCoord.xyz;

//    vec3 look=pushconstants.dx*(float(gl_FragCoord.x)/1440.0f-1.0f);
//    vec3 look=pushconstants.dy*(float(-gl_FragCoord.y)/900.0f+1.0f);
    vec3 nlook=normalize(look);
//    color=vec4(sin(look.x*100.0f), sin(look.y*100.0f), sin(look.z*100.0f), 1.0f);
//    color=vec4(0.2f, 0.2f, nlook.y, 1.0f);
//    color=vec4(nlook.x, nlook.y, nlook.z, 1.0f);
//    color=vec4(sin(look.x/1000.0f), sin(look.y/1000.0f), sin(look.z*10.0f), 1.0f);
    color=vec4((nlook.x+1.0f)/2.0f, (nlook.y+1.0f)/2.0f, (nlook.z+1.0f)/2.0f, 1.0f);
    color=vec4((-nlook.y+1.0f)/2.0f, (-nlook.y+1.0f)/2.0f, 0.9f, 1.0f);
    //this stuff still looks a little slippy, so maybe come back later and redo the math/make adjustments
//    color=vec4(float(gl_FragCoord.x)/2880.0f, 0, float(gl_FragCoord.y)/1800.0f, 1.0f);
//    color=vec4(float(gl_FragCoord.x)/1440.0f-1.0f, 0, float(gl_FragCoord.y)/900.0f-1.0f, 1.0f);
}
