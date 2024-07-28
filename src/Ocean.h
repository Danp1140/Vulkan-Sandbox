//
// Created by Daniel Paavola on 8/5/21.
//

#ifndef VULKANSANDBOX_OCEAN_H
#define VULKANSANDBOX_OCEAN_H

#include "Mesh.h"

#define OCEAN_PRE_TESS_SUBDIV 32

typedef struct OceanPushConstants {
	glm::mat4 cameravpmatrices = glm::mat4(1);
	alignas(16) glm::vec3 cameraposition = glm::vec3(0);
	uint32_t numlights = 0;
} OceanPushConstants;

class Ocean : public Mesh {
private:
	glm::vec2 bounds;
	Mesh*shore;
	// height & normal are duplicate with Mesh's own textures...why???
	TextureInfo heightmap, normalmap, velocitymap;
	VkDescriptorSet computeds;
	VkCommandBuffer* computecommandbuffers;
	TextureInfo seabeddepthmap;
	OceanPushConstants pushconstants;

	void initDescriptorSets (const VkDescriptorSetLayout objdsl) override;

	void initComputeCommandBuffers ();

public:
	/*
	 * Constructors & Destructors
	 */
	Ocean (glm::vec3 pos, glm::vec2 b, Mesh* s);

	/*
	 * Member Access
	 */
	VkDescriptorSet getComputeDescriptorSet () const {return computeds;}
	VkCommandBuffer* getComputeCommandBuffers () {return computecommandbuffers;}
	TextureInfo* getHeightMapPtr () {return &heightmap;}
	TextureInfo* getNormalMapPtr () {return &normalmap;}
	TextureInfo* getDepthMapPtr () {return &seabeddepthmap;}
	OceanPushConstants* getPushConstantsPtr () {return &pushconstants;}

	void renderDepthMap (Mesh* seabed);
	void rewriteTextureDescriptorSets () override;

	/*
	 * Vulkan Utilities
	 */
	static void createComputePipeline ();
	static void createGraphicsPipeline ();
	static void recordDraw (cbRecData data, VkCommandBuffer& cb);
	static void recordCompute (cbRecData data, VkCommandBuffer& cb);
	};


#endif //VULKANSANDBOX_OCEAN_H
