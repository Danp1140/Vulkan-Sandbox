//
// Created by Daniel Paavola on 12/25/22.
//

#ifndef VULKANSANDBOX_TERRAIN_H
#define VULKANSANDBOX_TERRAIN_H

#include "Mesh.h"

#define NODE_SIZE (sizeof(uint) * 16)
//#define NODE_SIZE (sizeof(uint) * 11)
#define VOXEL_SIZE sizeof(uint)
#define NODE_HEAP_SIZE 1024

/*
 * I'm unsure if Mesh inheritance is the right call here. Terrain generation can and will run independently of the mesh
 * that's actually showing up, so perhaps it would be better if Terrain owned a Mesh.
 */
class Terrain : private Mesh {
private:
	VkBuffer treesb = VK_NULL_HANDLE, voxelsb = VK_NULL_HANDLE;
	VkDeviceMemory tsbmemory = VK_NULL_HANDLE, vsbmemory = VK_NULL_HANDLE;
	uint16_t numnodes, numleaves, numvoxels;
	VkDescriptorSet* computedss;
public:
	Terrain ();

	static void recordCompute (cbRecData data, VkCommandBuffer& cb);

	static void recordTroubleshootDraw (cbRecData data, VkCommandBuffer& cb);

	uint16_t getNumLeaves () {return numleaves;}

	VkDescriptorSet getDS (uint8_t scii) {return computedss[scii];}
};


#endif //VULKANSANDBOX_TERRAIN_H
