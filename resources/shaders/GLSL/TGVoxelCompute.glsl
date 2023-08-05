#version 460 core

#define FREE -1u
#define AIR 0
#define WATER 1

#define XAXIS 0
#define YAXIS 1
#define ZAXIS 2

// test macro to make node access faster
#define DEREF(pnode) tree.nodes[pnode]

layout(local_size_x=1, local_size_y=1) in;

// if a block is free, parent stores the length of the free block (IN TERMS OF NODE IDXS, NOT RAW SIZE)
// perhaps rethink where we store free-ness
// could use 0u as a reserved value, since each block is guarenteed to have a root at 0u
struct Node {
    uint parent;
    uint voxel;
    uint children[8];   // child references stored with xyz as +++ -++ --+ +-+ ++- -+- --- +--
    uint childidx; // stores which child this node is of its parent
};

struct Voxel {
    uint material;
};

// could alias layout
layout (std430, set=0, binding=0) buffer Tree {
    Node nodes[];
} tree;

layout (std430, set=0, binding=1) buffer VoxelBuffer {
    Voxel voxels[];
} voxelbuffer;

// could add efficiency here and in subdiv(1) w/ multiple concurrent allocs
uint allocNode () {
    // ideally store as global variable
    uint firstfree;
    for (firstfree = 0; tree.nodes[firstfree].voxel != FREE; firstfree++) {}
    tree.nodes[firstfree].voxel = AIR;
    tree.nodes[firstfree + 1].voxel = FREE;
    tree.nodes[firstfree + 1].parent = tree.nodes[firstfree].parent - 1;
    return firstfree;
}

uint lowerLeftMost(uint pparent) {
	uint presult = pparent;
	for (int i = 0; i < 8; i++) {
		if (tree.nodes[presult].children[i] != -1u) {
			presult = tree.nodes[presult].children[i];
			i = -1;
		}
	}
	return presult;
}

void subdiv (uint pparent) {
    uint pnode;
    for (uint i = 0; i < 8; i++) {
        pnode = allocNode();
        tree.nodes[pparent].children[i] = pnode;
        tree.nodes[pnode].parent = pparent;
        tree.nodes[0].voxel = 0u;
        tree.nodes[0].childidx = i;
    }
}

void setupTree (uint depth) {
    tree.nodes[0].parent = -1u;
    tree.nodes[0].voxel = 0u;
    tree.nodes[0].childidx = -1u;
    // for each depth, traverse all leaves and call sudivide on each
    // since we subdivide in all dimensions, we may assume every node has all children
    /*for (uint d = 1; d < depth; d++) {
	// TODO: rename plowerleft to temp or smth
        uint plowerleft = lowerLeftMost(0u);
	//subdiv(tree.nodes[plowerleft].parent);	
	while (plowerleft != -1u) {
		for (uint i = 0; i < 8; i++) {
			subdiv(tree.nodes[tree.nodes[plowerleft].parent].children[i]);
		}
		if (DEREF(DEREF(plowerleft).parent).parent != -1u) {
			plowerleft = lowerLeftMost(DEREF(DEREF(DEREF(plowerleft).parent).parent).children[DEREF(DEREF(plowerleft).parent).childidx + 1]);
		}
	}
    }
    */
}

/* returns a pointer to the node next to v in the direction of dir. returns -1u if there is no node in that direction.
 * presumed to start on a leaf
 */
uint getNeighbor(uint v, bvec3 dir) {
    /*
    if (v + dir is sibling) {
        proceed to that sibling
        proceed in -dir til leaf
    }
    else (go up, etc)
    */
}

/*
 * eventually we will need a barrier after this compute/before troubleshooting/meshing shaders
 */
void main () {
    // temporary static-size (4x4x4) initialization
    if (tree.nodes[0].parent == 0u) {
        // setup root and voxel buffer
        voxelbuffer.voxels[0].material = AIR;
        voxelbuffer.voxels[1].material = WATER;
	setupTree(1u);
        tree.nodes[0].parent = -1u;
        tree.nodes[0].voxel = 0u;
        tree.nodes[0].childidx = -1u;
        // setup depth 1
        for (uint x = 0; x < 8; x++) {
            tree.nodes[x + 1].parent = 0u;
            tree.nodes[x + 1].voxel = (x + 1) % 2;
            tree.nodes[tree.nodes[x + 1].parent].children[x] = x + 1;
            tree.nodes[x + 1].childidx = x;
            // setup depth 2
            for (uint y = 0; y < 8; y++) {
                // for traversal testing, depth 1 child 1 gets no children
                /*if (x == 0) {
                    tree.nodes[x + 1].children[y] = -1u;
                    continue;
                }*/
                tree.nodes[x * 8 + y + 9].parent = x + 1;
                tree.nodes[x * 8 + y + 9].voxel = (x * 8 + y + 9) % 2;
                tree.nodes[tree.nodes[x * 8 + y + 9].parent].children[y] = x * 8 + y + 9;
                tree.nodes[x * 8 + y + 9].childidx = y;
                // end before depth 3
                for (uint z = 0; z < 8; z++) tree.nodes[x * 8 + y + 9].children[z] = -1u;
            }
        }
    }

    voxelbuffer.voxels[0].material = 0;
    if (voxelbuffer.voxels.length() > 0) voxelbuffer.voxels[0].material = 1;

    // for the time being, i'm going to combine the mesh gen and voxel gen in the same shader
}

uint getChildPtr(uint nodeptr, bvec3 oct) {
    if (oct.x && oct.y && oct.z) return tree.nodes[nodeptr].children[0];
    else return tree.nodes[nodeptr].children[7];
}
