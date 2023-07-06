//
// Created by Daniel Paavola on 12/25/22.
//

#include "Terrain.h"

Terrain::Terrain () : Mesh() {
	// starting w/ an arbitrary 4x4x4 (3 subdiv) all referencing the same 2 voxels
	// numnodes = 73u;
	numnodes = 66u;
	numleaves = 57u;
	numvoxels = 2u;
//	GraphicsHandler::VKSubInitStorageBuffer()
	// would use VKSubInitStorageBuffer, but that requires as many sbs as scis
	// could be experiencing trouble w/ not rounding up padding correctly
	// best course of action is likely to make VKSubInitStorageBuffer more general (dw, refator will be small cuz we
	// only ever use it once lol)
//	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&treesb,
//													 NODE_SIZE * numnodes,
//													 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
//													 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
//													 &tsbmemory,
//													 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&voxelsb,
//													 VOXEL_SIZE * numvoxels,
//													 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
//													 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
//													 &vsbmemory,
//													 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkDescriptorSetLayout layoutstemp[GraphicsHandler::vulkaninfo.numswapchainimages];
	for (uint32_t x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++)
		layoutstemp[x] = GraphicsHandler::vulkaninfo.terraingencomputepipeline.objectdsl;
	computedss = new VkDescriptorSet[GraphicsHandler::vulkaninfo.numswapchainimages];
	VkDescriptorSetAllocateInfo descriptorsetallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			GraphicsHandler::vulkaninfo.numswapchainimages,
			&layoutstemp[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &descriptorsetallocinfo,
							 &computedss[0]);
	GraphicsHandler::VKSubInitStorageBuffer(&computedss[0], 0, NODE_SIZE, numnodes, &treesb, &tsbmemory);
	GraphicsHandler::VKSubInitStorageBuffer(&computedss[0], 1, VOXEL_SIZE, numvoxels, &voxelsb, &vsbmemory);
	VkDescriptorBufferInfo descbuffinfo;
	VkWriteDescriptorSet writetemp {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
									nullptr,
									VK_NULL_HANDLE,
									0,
									0,
									1,
									VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
									nullptr,
									&descbuffinfo,
									nullptr};
	for (uint8_t x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++) {
		writetemp.dstSet = computedss[x];
		writetemp.dstBinding = 0;
		descbuffinfo = {treesb, 0, NODE_SIZE * numnodes};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
							   1,
							   &writetemp,
							   0,
							   nullptr);
		writetemp.dstBinding = 1;
		descbuffinfo = {treesb, 0, VOXEL_SIZE * numvoxels};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
							   1,
							   &writetemp,
							   0,
							   nullptr);
	}
}

void Terrain::recordCompute (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			VK_NULL_HANDLE,
			-1u,
			VK_NULL_HANDLE,
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			0,
			&cmdbufinherinfo
	};
	vkBeginCommandBuffer(cb, &cmdbufbegininfo);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, data.pipeline.pipeline);
	vkCmdBindDescriptorSets(cb,
							VK_PIPELINE_BIND_POINT_COMPUTE,
							data.pipeline.pipelinelayout,
							0,
							1,
							&data.descriptorset,
							0,
							nullptr);
	vkCmdDispatch(cb, 73, 1, 1);
	vkEndCommandBuffer(cb);
}

void Terrain::recordTroubleshootDraw (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			data.renderpass,
			0,
			data.framebuffer,
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
			&cmdbufinherinfo
	};
	vkBeginCommandBuffer(cb, &cmdbufbegininfo);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipeline.pipeline);
	vkCmdBindDescriptorSets(cb,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							data.pipeline.pipelinelayout,
							0,
							1,
							&data.descriptorset,
							0,
							nullptr);
	vkCmdPushConstants(cb,
					   data.pipeline.pipelinelayout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(glm::mat4),
					   data.pushconstantdata);
	vkCmdDraw(cb, data.numtris * 3u, 1, 0, 0);
	vkEndCommandBuffer(cb);
}