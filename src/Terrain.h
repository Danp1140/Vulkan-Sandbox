//
// Created by Daniel Paavola on 12/25/22.
//

#ifndef VULKANSANDBOX_TERRAIN_H
#define VULKANSANDBOX_TERRAIN_H

#include "Mesh.h"

#define NODE_SIZE (sizeof(uint) * 16)
//#define NODE_SIZE (sizeof(uint) * 11)
#define VOXEL_SIZE sizeof(uint)
#define NODE_HEAP_SIZE 4096
#define ALLOC_INFO_BUF_SIZE 128

typedef struct Node {
	uint32_t parent, voxel, children[8], childidx;
} Node;

/*
 * I'm unsure if Mesh inheritance is the right call here. Terrain generation can and will run independently of the mesh
 * that's actually showing up, so perhaps it would be better if Terrain owned a Mesh.
 */
class Terrain : private Mesh {
private:
	VkBuffer treesb = VK_NULL_HANDLE, voxelsb = VK_NULL_HANDLE, meminfosb = VK_NULL_HANDLE;
	VkDeviceMemory tsbmemory = VK_NULL_HANDLE, vsbmemory = VK_NULL_HANDLE, msbmemory;
	uint32_t numnodes, numleaves, numvoxels;
	VkDescriptorSet* computedss;
	bool memoryavailable; // flag to coordinate CPU node heap realloc & GPU memory use
	uint8_t compfif; // fif compute was submitted on, to track when memavail should be reset
	VkDeviceSize nodeheapfreesize; // troubleshooting measure to make sure heap is safe

	void setupHeap();
public:
	Terrain ();

	static void createComputePipeline ();

	static void createGraphicsPipeline ();

	static void recordCompute (cbRecData data, VkCommandBuffer& cb);

	static void recordTroubleshootDraw (cbRecData data, VkCommandBuffer& cb);

	uint32_t getNumLeaves () {return numleaves;}

	void* getNodeHeapPtr ();

	void updateNumLeaves ();

	glm::vec3 getLeafPos (uint32_t leafidx, uint32_t& nodeidx, Node* heap);

	void subdivide (uint32_t idx, Node* heap);

	void collapse (uint32_t parentidx, Node* heap);

	void freeVoxel(uint32_t ptr, Node* heap);

	uint32_t allocVoxel(Node* heap);

	void updateVoxels (glm::vec3 camerapos);

	void dumpHeap (Node* heap, uint32_t n);

	VkDescriptorSet getDS (uint8_t scii) {return computedss[scii];}

	VkBuffer getTreeBuffer () {return treesb;}

	bool isMemoryAvailable() {return memoryavailable;}
	void setMemoryAvailable(bool ma) {memoryavailable = ma;}

	uint8_t getCompFIF() {return compfif;}
	void setCompFIF(uint8_t cf) {compfif = cf;}

	VkDeviceSize getNodeHeapFreeness() {return nodeheapfreesize;}
};


#endif //VULKANSANDBOX_TERRAIN_H
