#version 460 core

#define ASPECT_RATIO (144. / 90.)
#define ARSF (1. / ASPECT_RATIO)
#define SCALE 0.1
#define VOXEL_BOUNDS 10.


#define DEREF(pnode) tree.nodes[pnode]

struct Node {
    uint parent;
    uint voxel;
    uint children[8];   // child references stored with xyz as +++ -++ --+ +-+ ++- -+- --- +--
    uint childidx;
};

layout (std430, set=0, binding=0) buffer Tree {
    Node nodes[];
} tree;

struct NodeRelation {
    uint nodeidx;
    uint childidx;
};

const vec2 vertexpositions[4]={
    SCALE*vec2(-ARSF, 1.),
    SCALE*vec2(ARSF, 1.),
    SCALE*vec2(ARSF, -1.),
    SCALE*vec2(-ARSF, -1.)
};
const uint vertexindices[6]={
    0, 1, 2,
    2, 3, 0
};

layout(push_constant) uniform PushConstants{
    mat4 cameravp;
} constants;

layout(location=0) out uint voxelid;
layout(location=1) out uint nodeid;

// TODO: update these function descriptions

uint siblingToRight(uint nodeidx);

uint bottomLeftMost(uint nodeidx);

uint parentToRight(uint nodeidx);

vec3 childDirection(uint childidx);

vec3 traversalposition = vec3(0.);
uint traversaldepth = 0u;

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

void main() {
    const uint destinationidx = uint(floor(float(gl_VertexIndex) / 6.f));
    uint traversalidx = 0u;
    uint currentnode = 0u;
    uint tempnode;

    /*
    currentnode = bottomLeftMost(0u);           // start by going all the way to the bottom left
    while (traversalidx != destinationidx) {
        tempnode = siblingToRight(currentnode);
        if (tempnode != -1u) {  // if theres a sibling to the right:
            traversalposition -= childDirection(tree.nodes[currentnode].childidx) * pow(0.5, traversaldepth);
            traversalposition += childDirection(tree.nodes[tempnode].childidx) * pow(0.5, traversaldepth);
            currentnode = tempnode; // traverse to that sibling
        }
        else {  // if theres no sibling to the right
            if (currentnode != 0u) {    // if we're not at the root
                tempnode = parentToRight(currentnode);  // go up and to the right searching for the next parent
                if (tempnode == -1u) break; // if that takes us to the root, we're done
		// below is new code in an attempt to avoid infinite loop
		// so i had to disable this to make anything more than quad 1 work, im guessing itll crash again when i dont have a perfect square tessellation
		// ^^^ lmaoooooo it immediately started crashing again
		// it is clear though that this fix does not address the actual problem
		// the issue is an infinie loop (what distinugishes a GPU timeout from an OS crash is a mystery to me though)
		// one starting point is modifying functions to conform to our all-or-nothing subdiv strategy
		// alr nvm it keeps crashing even after i re-added it, maybe the issue is elsewhere...i dont remember editing the compute tho...
		// crashes looked like a hang, but also vram out of range, indicating that loop is running out of control, could be affected by heap size tho
		// could try a 0-depth tree just to see what happens
		// setupTree(0u) and setupTree(1u) (w/ 64 leaves in c++ & 512 nodes) didn't crash)
		/*
		else {
			traversalposition = vec3(0., 0.5, 0.);
			break;
		}
		*/
            /*{
        }
        currentnode = bottomLeftMost(tempnode); // go to the bottom left no matter what
        traversalidx++;
    }
*/

    getLeafPos(destinationidx, traversalposition, currentnode);
    voxelid = tree.nodes[currentnode].voxel;
    nodeid = currentnode;
    nodeid = destinationidx;
    gl_Position = 0.5 * vec4(vertexpositions[vertexindices[gl_VertexIndex - 6 * destinationidx]], 0., 0.) + constants.cameravp * vec4(traversalposition, 1.);
    //gl_Position = float(nodeid) / 64. * vec4(vertexpositions[vertexindices[gl_VertexIndex - 6 * destinationidx]], 0., 0.) + constants.cameravp * vec4(traversalposition, 1.);
//    gl_Position = vec4(vertexpositions[vertexindices[gl_VertexIndex - 6 * destinationidx]], 0., 0.) + constants.cameravp * vec4(destinationidx * 0.1, 0., 0., 1.);
}

uint siblingToRight(uint nodeidx) {
    if (tree.nodes[nodeidx].parent == -1u) return -1u;
    uint tempidx;
    for (uint siblingidx = tree.nodes[nodeidx].childidx + 1; siblingidx < 8; siblingidx++) {
        tempidx = tree.nodes[tree.nodes[nodeidx].parent].children[siblingidx];
        if (tempidx != -1u) return tempidx;
//        // return expression was recognized as device uint4 by vk compiler, so idk whats happening
//        uint tempcuzvksucks = tree.nodes[tree.nodes[nodeidx].parent].children[siblingidx];      // check this line, its very important
//        if (tempcuzvksucks != -1u) return tempcuzvksucks;
    }
    return -1u;
}

uint bottomLeftMost(uint nodeidx) {
    uint result = nodeidx;
    while (tree.nodes[result].children[0] != -1u) {
	result = tree.nodes[result].children[0];
	traversaldepth++;
	traversalposition += childDirection(0) * pow(0.5, traversaldepth);
    }
    return result;
    // below is code for case without guarenteed complete tree levels
	/*
    uint result = nodeidx;
    for (int x = 0; x < 8; x++) {
        if (tree.nodes[result].children[x] != -1) {
            result = tree.nodes[result].children[x];
            traversaldepth++;
            traversalposition += childDirection(x) * pow(0.5, traversaldepth);
            x = -1;
        }
    }
    return result;
    */
}

uint parentToRight(uint nodeidx) {
//    // need end-cases later
//    return siblingToRight(tree.nodes[nodeidx].parent);
    // position changes assume the above use of parentToRight exclusively
    if (nodeidx == -1u) return -1u; // line seems unneccesary
    uint parent = tree.nodes[nodeidx].parent, nextparent = siblingToRight(parent);
    traversalposition -= childDirection(tree.nodes[nodeidx].childidx) * pow(0.5, traversaldepth);
    traversaldepth--;
    traversalposition -= childDirection(tree.nodes[parent].childidx) * pow(0.5, traversaldepth);
    traversalposition += childDirection(tree.nodes[nextparent].childidx) * pow(0.5, traversaldepth);
    while (nextparent == -1u) {
        if (parent == 0) return -1u;
        traversalposition -= childDirection(tree.nodes[parent].childidx) * pow(0.5, traversaldepth);
//        traversaldepth--;
        parent = tree.nodes[parent].parent;
        nextparent = siblingToRight(parent);
    }
    return nextparent;
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
