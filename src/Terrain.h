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
	VkBuffer treesb = VK_NULL_HANDLE, voxelsb = VK_NULL_HANDLE;
	VkDeviceMemory tsbmemory = VK_NULL_HANDLE, vsbmemory = VK_NULL_HANDLE;
	uint32_t numnodes, numleaves, numvoxels;
	VkDescriptorSet* computedss;
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

	void updateVoxels (glm::vec3 camerapos);

	VkDescriptorSet getDS (uint8_t scii) {return computedss[scii];}

	VkBuffer getTreeBuffer () {return treesb;}

};


#endif //VULKANSANDBOX_TERRAIN_H
