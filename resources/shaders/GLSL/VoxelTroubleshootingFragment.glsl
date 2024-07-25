#version 460 core

struct Node {
    uint parent;
    uint voxel;
    uint children[8];   // child references stored with xyz as +++ -++ --+ +-+ ++- -+- --- +--
    uint childidx;
};

struct Voxel {
    uint material;
};

// should double-check the correct memory layout here
layout (std430, set=0, binding=0) buffer Tree {
    Node nodes[];
} tree;

layout (std430, set=0, binding=1) buffer VoxelBuffer {
    Voxel voxels[];
} voxelbuffer;

layout(location=0) flat in uint voxelid;
layout(location=1) flat in uint nodeid;

layout(location=0) out vec4 color;

void main () {
    //color = vec4(1., 0., 0., 1.) * float(nodeid) / 256.;
    //color.w = 1.;
    /*
    if (voxelbuffer.voxels[voxelid].material == 0) color = vec4(1., 0., 0., 1.);
    else if (voxelbuffer.voxels[voxelid].material == 1) color = vec4(0., 1., 0., 1.);
    else color = vec4(0., 0., 1., 1.);
    */
    if (voxelid == 0) color = vec4(1., 0., 0., 1.);
    else if (voxelid == 1) color = vec4(0., 1., 0., 1.);
    else if (voxelid == 2) color = vec4(1., 1., 0., 1.);
    else if (voxelid == 3) color = vec4(1., 0., 1., 1.);
    else if (voxelid == 4) color = vec4(0., 1., 1., 1.);
    else color = vec4(0., 0., 1., 1.);
}
