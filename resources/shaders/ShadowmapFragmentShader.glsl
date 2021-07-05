#version 410 core

void main() {
    //technically unneccesary, but here for troubleshooting via custom scaling
    gl_FragDepth=gl_FragCoord.z;
}
