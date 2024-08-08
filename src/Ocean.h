//
// Created by Daniel Paavola on 8/5/21.
//

#ifndef VULKANSANDBOX_OCEAN_H
#define VULKANSANDBOX_OCEAN_H

#include "Mesh.h"
#include "Compositing.h"

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
	TextureInfo velocitymap;
	VkDescriptorSet computeds;
	TextureInfo seabeddepthmap;
	OceanPushConstants pushconstants;

	static PipelineInfo graphicspipeline, computepipeline;

	void initDescriptorSets (const VkDescriptorSetLayout objdsl) override;

public:
	/*
	 * Constructors & Destructors
	 */
	Ocean (glm::vec3 pos, glm::vec2 b, Mesh* s);

	/*
	 * Member Access
	 */
	VkDescriptorSet getComputeDescriptorSet () const {return computeds;}
	TextureInfo* getDepthMapPtr () {return &seabeddepthmap;}
	OceanPushConstants* getPushConstantsPtr () {return &pushconstants;}

	void renderDepthMap (Mesh* seabed);
	void rewriteTextureDescriptorSets () override;

	/*
	 * Vulkan Utilities
	 */
	static void init ();
	static void terminate ();
	static void createComputePipeline ();
	static void createGraphicsPipeline ();
	static const PipelineInfo& getGraphicsPipeline () {return graphicspipeline;}
	static const PipelineInfo& getComputePipeline () {return computepipeline;}
	static void recordDraw (cbRecData data, VkCommandBuffer& cb);
	static void recordCompute (cbRecData data, VkCommandBuffer& cb);
};

#endif //VULKANSANDBOX_OCEAN_H
