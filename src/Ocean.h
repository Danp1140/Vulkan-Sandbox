//
// Created by Daniel Paavola on 8/5/21.
//

#ifndef VULKANSANDBOX_OCEAN_H
#define VULKANSANDBOX_OCEAN_H

#include "Mesh.h"

#define OCEAN_PRE_TESS_SUBDIV 32

//why do we need "public"?
class Ocean:public Mesh{
private:
	glm::vec2 bounds;
	Mesh*shore;
	TextureInfo heightmap, normalmap, velocitymap;
	VkDescriptorSet* computedescriptorsets;
	VkCommandBuffer* computecommandbuffers;
	TextureInfo seabeddepthmap;

	void initDescriptorSets ();

	void initComputeCommandBuffers ();

public:
	Ocean (glm::vec3 pos, glm::vec2 b, Mesh* s);

	static void createComputePipeline ();

	static void createGraphicsPipeline ();

	void renderDepthMap (Mesh* seabed);

	void rewriteTextureDescriptorSets ();

	void recordDraw (uint8_t fifindex, uint8_t sciindex, VkDescriptorSet* sceneds);

	void recordCompute (uint8_t fifindex);

	void rewriteDescriptorSet (uint32_t index);

	VkDescriptorSet* getComputeDescriptorSets () {return computedescriptorsets;}

	VkCommandBuffer* getComputeCommandBuffers () {return computecommandbuffers;}

	TextureInfo*
	getHeightMapPtr () {return &heightmap;}   //perhaps use displacement map instead (better for cresting and such)
	TextureInfo* getNormalMapPtr () {return &normalmap;}

	TextureInfo* getDepthMapPtr () {return &seabeddepthmap;}
};


#endif //VULKANSANDBOX_OCEAN_H
