#version 460 core

#define INFO -2u
#define FREE -1u
#define AIR 0
#define WATER 1
#define ROCK 2

#define XAXIS 0
#define YAXIS 1
#define ZAXIS 2

#define VOXEL_BOUNDS 10

#define DEREF(pnode) tree.nodes[pnode]
#define NEXT_FREE_DIST(pnode) tree.nodes[pnode].parent
#define IS_FREE(pnode) tree.nodes[pnode].voxel == FREE

#define INFO_BLOCK tree.nodes[constants.heapsize - 2u]
#define FIRST_FREE_PTR INFO_BLOCK.parent
#define HEAP_RESIZE_FLAG_PTR INFO_BLOCK.childidx
#define HEAP_RESIZE_FLAG DEREF(HEAP_RESIZE_FLAG_PTR)

layout(constant_id=0) const uint HEAP_SIZE = 1;
layout(constant_id=1) const uint ALLOC_INFO_BUF_SIZE = 1;

layout(local_size_x=1, local_size_y=1) in;

layout(push_constant) uniform PushConstants {
	uint numleaves, heapsize, phase;
	mat4 cameravp;
	vec3 camerapos;
}constants;

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
layout (std430, set=0, binding=0) coherent buffer Tree {
    Node nodes[];
} tree;

layout (std430, set=0, binding=1) buffer VoxelBuffer {
    Voxel voxels[];
} voxelbuffer;

// first half is indices of cpu-allocd blocks
// second half is indices of blocks to be freed by cpu
// allocptr points to next available cpu-allocd block
// freeptr points to next available slot to request free
layout (std430, set=0, binding=2) buffer MemoryInfoBuffer {
	uint allocptr, freeptr, numtoalloc, addrs[ALLOC_INFO_BUF_SIZE];
} meminfo;

vec3 childDirection(uint childidx) {
    if (childidx == 0) return vec3(1., 1., 1.);
    if (childidx == 1) return vec3(-1., 1., 1.);
    if (childidx == 2) return vec3(-1., -1., 1.);
    if (childidx == 3) return vec3(1., -1., 1.);
    if (childidx == 4) return vec3(1., 1., -1.);
    if (childidx == 5) return vec3(-1., 1., -1.);
    if (childidx == 6) return vec3(-1., -1., -1.);
    return vec3(1., -1., -1.);
}

// this function presumes a valid leafidx (i.e. less than the number of leaves
void getLeafPos(in uint leafidx, out vec3 position, out uint nodeidx) {
	uint tempptr = 0u, depth = 0u;
	position = vec3(0.);
	// go to bottom leftmost
	while (DEREF(tempptr).children[0] != -1u) {
		tempptr = DEREF(tempptr).children[0];
		depth++;
		position += vec3(1.) * pow(0.5, depth);
	}
	uint childidx = 0u;
	for (uint i = 0u; i < leafidx; i++) {
		// if we can continue on the same level, do so
		if (childidx != 7) {
			position -= childDirection(childidx) * pow(0.5, depth);
			childidx++;
			position += childDirection(childidx) * pow(0.5, depth);
			tempptr = DEREF(DEREF(tempptr).parent).children[childidx];
			// go BLM just in case
			while (DEREF(tempptr).children[0] != -1u) {
				tempptr = DEREF(tempptr).children[0];
				depth++;
				position += vec3(1.) * pow(0.5, depth); 
				childidx = 0u;
			}
			
		}
		// if we have reached the end of the level
		else {
			// step up until we find a sibling to the right
			while (childidx == 7) {
				position -= childDirection(childidx) * pow(0.5, depth);
				depth--;
				tempptr = DEREF(tempptr).parent;
				childidx = DEREF(tempptr).childidx;
			}
			// go to that sibling
			position -= childDirection(childidx) * pow(0.5, depth);
			childidx++;
			position += childDirection(childidx) * pow(0.5, depth);
			tempptr = DEREF(DEREF(tempptr).parent).children[childidx];
			// go BLM
			while (DEREF(tempptr).children[0] != -1u) {
				tempptr = DEREF(tempptr).children[0];
				depth++;
				position += vec3(1.) * pow(0.5, depth); 
				childidx = 0u;
			}
		}
	}
	nodeidx = tempptr;
}

// for now just takes voxel value directly from parent
void subdivide (uint ptr) {
	// signal that we're gonna need more memory
	atomicAdd(meminfo.numtoalloc, 1);
	if (meminfo.addrs[meminfo.allocptr] == -1u || meminfo.allocptr == ALLOC_INFO_BUF_SIZE/2 - 1) return;

	// if we still have some for this frame, use it
	// should p consider what happens if we end up with excess memory
	uint freeblock = atomicAdd(meminfo.allocptr, 1);
	uint tempptr;
	
	for (uint i = 0; i < 8; i++) {
		tempptr = freeblock + i;
		DEREF(ptr).children[i] = tempptr;
		DEREF(tempptr).parent = ptr;
		DEREF(tempptr).voxel = DEREF(ptr).voxel;
		DEREF(tempptr).childidx = i;
		for (uint j = 0; j < 8; j++) DEREF(tempptr).children[j] = -1u;
	}
	atomicAdd(DEREF(0).childidx, 7u);
}

void main () {
// if we move ahead with phase system init could be its own phase, would be a much safer way to do things
	// since we started setup on the CPU, i dont think we even need this anymore??
	if (tree.nodes[0].parent == 0u) {
		voxelbuffer.voxels[0].material = AIR;
		voxelbuffer.voxels[1].material = WATER;
		voxelbuffer.voxels[2].material = ROCK;
		//DEREF(0).parent = -1u;
		//DEREF(0).childidx = 1u;
		//DEREF(0).voxel = AIR;
		//for (uint i = 0; i < 8; i++) DEREF(0).children[i] = -1u;
		meminfo.allocptr = 0u;
		meminfo.numtoalloc = 0u;
		meminfo.freeptr = 0u;

	}
	else {
		vec3 leafpos;
		uint leafidx;
		getLeafPos(gl_GlobalInvocationID.x, leafpos, leafidx);
		uint depth = 0u;
		uint tempptr = leafidx;
		while (tempptr != 0u) {
			depth++;
			tempptr = DEREF(tempptr).parent;
		}
		float dist = distance(constants.camerapos, leafpos);
		if (dist < 2. && depth < 2) {
			DEREF(leafidx).voxel = 3;
			subdivide(leafidx);
		}
		else if (dist > 3.) {
			DEREF(leafidx).voxel = 0;
		}
	}
}
