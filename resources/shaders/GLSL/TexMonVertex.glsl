#version 460 core

#define ASPECT_RATIO (144./90.)
#define SCALE 0.5
#define OFFSET vec2(-0.5, 0.)

const vec2 vertexpositions[4]={
    OFFSET+SCALE*vec2(-1., 1.*ASPECT_RATIO),
    OFFSET+SCALE*vec2(0., 1.*ASPECT_RATIO),
    OFFSET+SCALE*vec2(0., 0.),
    OFFSET+SCALE*vec2(-1., 0.)
};
const vec2 vertexuvs[4]={
    vec2(0., 0.),
    vec2(1., 0.),
    vec2(1., 1.),
    vec2(0., 1.)
};
const uint vertexindices[6]={
    0, 1, 2,
    2, 3, 0
};

layout(location=0) out vec2 uvthru;

void main(){
    gl_Position=vec4(vertexpositions[vertexindices[gl_VertexIndex]], 0., 1.);
    uvthru=vertexuvs[vertexindices[gl_VertexIndex]];
}
