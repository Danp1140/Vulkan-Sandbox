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
#define DEALLOC_LOCK INFO_BLOCK.children[0]

// avoiding compilation errors until we finish pipeline creation refactor
/*
layout(constant_id=0) const uint HEAP_SIZE;
layout(constant_id=1) const uint ALLOC_INFO_BUF_SIZE;
*/

layout(local_size_x=1, local_size_y=1) in;

layout(push_constant) uniform PushConstants {
	uint numleaves, heapsize, phase;
	mat4 cameravp;
	vec3 camerapos;
}constants;

// redoing heap. in free block, parent stores length to next free block
// could use 0u as a reserved value, since each block is guarenteed to have a root at 0u
// okay also for root node (DEREF(0)), childidx stores # leaves
//
// should be storing a doubly-linked list of free blocks you idiot
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

void initFreeList2() {
	for (uint i = 1; i < constants.heapsize - 1u; i++) {
		DEREF(i).voxel = FREE;
		NEXT_FREE_DIST(i) = 1u;
	}
	INFO_BLOCK.voxel = INFO;
	FIRST_FREE_PTR = 1u;
	DEALLOC_LOCK = 0u;
}

// could add efficiency here and in subdiv(1) w/ multiple concurrent allocs
// seems like we need a way to return nullptr if we run out of space
// im gonna guess this function is experiencing sync issues, lending to hangs and misallocs
// the only synchronization option i see here is making firstfree a read/write value we change through atomic ops
// may have to keep a very explicit list of free/taken blocks to enable this
uint allocNode () {
    // ideally store as global variable
    uint firstfree;
    for (firstfree = 0; tree.nodes[firstfree].voxel != FREE; firstfree++) {}
    tree.nodes[firstfree].voxel = AIR;
    // as soon as we implement dealloc, gotta check length here instead of just setting next value
    if (DEREF(firstfree).parent > 1) {
	    tree.nodes[firstfree + 1].voxel = FREE;
	    tree.nodes[firstfree + 1].parent = tree.nodes[firstfree].parent - 1;
    }
    // eventually need a way to handle when this gets to 0 and free block is gone
    return firstfree;
}

uint allocNode2() {
	/*
	uint ptr;
	// me: i am going to have a very sober evening and create a surprisingly clean, if slightly bodgey, GPU heap allocator
	// me one (1) drink later:
	if (DEALLOC_LOCK != 0u) {
		ptr = constants.heapsize - 2u;
		while (DEREF(ptr).voxel != FREE) ptr--;
	}
	else ptr = atomicAdd(FIRST_FREE_PTR , NEXT_FREE_DIST(FIRST_FREE_PTR));
	*/
	uint ptr = atomicAdd(FIRST_FREE_PTR, NEXT_FREE_DIST(FIRST_FREE_PTR));
	DEREF(ptr).voxel = AIR; // set to anything, just to know its not free
				// notably, with out new system the concept of a "FREE" voxel could be obsolete as we know where the next free block is
	return ptr;
}

void deallocNode(uint ptr) {
	DEREF(ptr).voxel = FREE;
	uint blocklength = 1u;
	// should technically check if we're at the end of the heap
	if (DEREF(ptr + 1).voxel == FREE) {
		blocklength += DEREF(ptr + 1).parent;
	}
	if (ptr > 1 && DEREF(ptr - 1).voxel == FREE) {
		uint prevfree;
		for (prevfree = ptr - 1; prevfree != 0; prevfree--) {}
		DEREF(prevfree).parent += blocklength;	
	}
	else {
		DEREF(ptr).parent = blocklength;
	}
}

void deallocNode2(uint ptr) {
	DEREF(ptr).voxel = FREE;
	atomicExchange(DEALLOC_LOCK, uint(ptr < FIRST_FREE_PTR));	
	if (DEALLOC_LOCK != 0u) { // if this is the first free block
		NEXT_FREE_DIST(ptr) = FIRST_FREE_PTR - ptr;
		FIRST_FREE_PTR = ptr;
		atomicExchange(DEALLOC_LOCK, 0u);
	}
	else {
		// i believe this could still technically race, we'll see
		// many of the below could be made atomic ops, unsure if this is necessary
		uint tempptr = FIRST_FREE_PTR;
		while (tempptr + NEXT_FREE_DIST(tempptr) < ptr) tempptr += NEXT_FREE_DIST(tempptr);
		NEXT_FREE_DIST(ptr) = tempptr + NEXT_FREE_DIST(tempptr) - ptr;
		NEXT_FREE_DIST(tempptr) = ptr - tempptr;	
	}
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

// does nothing if called on a non-leaf
// for now they directly inherit their parent's voxel ptr
void subdiv (uint pparent) {
    if (DEREF(pparent).children[0] != -1u) return;
    uint pnode;
    for (uint i = 0; i < 8; i++) {
        // pnode = allocNode();
	pnode = allocNode2();
        tree.nodes[pparent].children[i] = pnode;
        tree.nodes[pnode].parent = pparent;
        tree.nodes[pnode].voxel = DEREF(pparent).voxel;
        tree.nodes[pnode].childidx = i;
	for (uint j = 0; j < 8; j++) {
		DEREF(pnode).children[j] = -1u;
	}
    }
    atomicAdd(DEREF(0).childidx, 7u);
}

void collapse (uint pparent) {
	atomicAdd(DEREF(0).childidx, -7);
	for (uint i = 0; i < 8; i++) {
		//deallocNode2(DEREF(pparent).children[i]);
		DEREF(pparent).children[i] = -1u;
	}
	//for (uint i = 0; i < 8; i++) DEREF(DEREF(pparent).children[i]).voxel++;
}

uint leafToRight(uint start) {
	uint temp = start;
	uint cidx = DEREF(temp).childidx;
	if (cidx == 7) {
		while (cidx == 7) {
			temp = DEREF(temp).parent;
			if (temp == 0u) return -1u;
			cidx = DEREF(temp).childidx;
		}
	}
	return lowerLeftMost(DEREF(DEREF(temp).parent).children[cidx + 1]);
}

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

/*
 * eventually we will need a barrier after this compute/before troubleshooting/meshing shaders
 */
void main () {
// if we move ahead with phase system init could be its own phase, would be a much safer way to do things
    if (tree.nodes[0].parent == 0u) {
	// initFreeList();
	//initFreeList2();
        voxelbuffer.voxels[0].material = AIR;
        voxelbuffer.voxels[1].material = WATER;
	voxelbuffer.voxels[2].material = ROCK;
	//DEREF(0).parent = -1u;
	DEREF(0).childidx = 1u;
	DEREF(0).voxel = AIR;
	for (uint i = 0; i < 8; i++) DEREF(0).children[i] = -1u;
	//subdiv(0u);
    }
    // down here runs on a per-node basis
    /*else if (constants.phase == 0) {
	vec3 voxpos;
	uint voxptr;
	getLeafPos(gl_GlobalInvocationID.x, voxpos, voxptr);
	const float r0 = 1./2., maxdepth = 5.; // depth of 2 for every 5 units away the camera is, maxing out at 5.
	float cameradist = distance(constants.camerapos, voxpos);
	uint depth = 0u;
	uint tempptr = voxptr;
	while (tempptr != 0u) {
		depth++;
		tempptr = DEREF(tempptr).parent;
	}
	const float c = 1.;
	float x = maxdepth - (r0 * cameradist) - float(depth);
	/*
	if (x > c) {
		DEREF(voxptr).voxel = 0; // & subdivide
		if (depth < 2) subdiv(voxptr);
	}
	else if (depth > 1 && x < -c) {
		DEREF(voxptr).voxel = 1; // & collapse
		bool collapse = true;
		for (uint i = 0; i < 8; i++) {
			if (DEREF(DEREF(DEREF(voxptr).parent).children[i]).voxel != 1) collapse = false;
		}
		if (collapse) supermult(DEREF(voxptr).parent);
	}
	else {
		DEREF(voxptr).voxel = 2; // do nothing, this is just for troubleshooting
	}
	*/
	/*if (cameradist < 3.) {
		DEREF(voxptr).voxel = 0;
		if (depth < 2) subdiv(voxptr);
	}
	else if (cameradist > 5.) {
		DEREF(voxptr).voxel = 1;

	}
	else DEREF(voxptr).voxel = 2;
    }*/
    /*else {
	vec3 voxpos;
	uint voxptr;
	getLeafPos(gl_GlobalInvocationID.x * 8, voxpos, voxptr);
	voxptr = DEREF(voxptr).parent;
	uint depth = 0u;
	uint tempptr = voxptr;
	while (tempptr != 0u) {
		depth++;
		tempptr = DEREF(tempptr).parent;
	}
	if (depth == 0) return;
	bool shouldcollapse = true;
	for (uint i = 0; i < 8; i++) {
		if (DEREF(DEREF(voxptr).children[i]).voxel != 1) {
			shouldcollapse = false;
			break;
		}
	}
	if (shouldcollapse) collapse(voxptr);
    }*/
}
