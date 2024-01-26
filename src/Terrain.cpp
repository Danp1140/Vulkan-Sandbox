//
// Created by Daniel Paavola on 12/25/22.
//

#include "Terrain.h"

Terrain::Terrain () : Mesh() {
	numnodes = 4096u; // not sure how we reconcile this with the NODE_HEAP_SIZE macro in the .h
	numleaves = 1u;
	numvoxels = 3u;

	GraphicsHandler::vulkaninfo.terraingenpushconstants.heapsize = numnodes;

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
//													 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

	/* doubly-linked list heap setup
	 * root's parent stores ptr to first free
	 * free blocks are 8 nodes long, with only the first node containing information as follows
	 * 	- voxel = -1u if free
	 * 	- parent stores ptr to next free
	 * 	- childidx stores ptr to prev free
	 * 	- above two are -1u if no such block exists
	 */
	void* heap;
	vkMapMemory(
			GraphicsHandler::vulkaninfo.logicaldevice,
			tsbmemory,
			0u,
			numnodes * NODE_SIZE,
			0,
			&heap);
	Node* nodeheap = reinterpret_cast<Node*>(heap);
	uint32_t i;
	for (i = 1; i < NODE_HEAP_SIZE; i += 8) {
		nodeheap[i].voxel = -1u;
		nodeheap[i].parent = i + 8;
		nodeheap[i].childidx = i - 8;
	}
	nodeheap[8].childidx = -1u;
	nodeheap[i - 8].parent = -1u;
	nodeheap[0].parent = 1u;
	subdivide(0u, nodeheap);
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, tsbmemory);
}

void Terrain::createComputePipeline () {
	VkDescriptorSetLayoutBinding objdslbindings[2] {{
															0,
															VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
															1,
															VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
															VK_SHADER_STAGE_FRAGMENT_BIT,
															nullptr
													}, {
															1,
															VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
															1,
															VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
															VK_SHADER_STAGE_FRAGMENT_BIT,
															nullptr
													}};
	VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {{
															   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
															   nullptr,
															   0,
															   0,
															   nullptr
													   }, {
															   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
															   nullptr,
															   0,
															   2,
															   &objdslbindings[0]
													   }};


	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	pii.shaderfilepathprefix = "tgvoxel";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TerrainGenPushConstants)};

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.terraingencomputepipeline, pii);
}

void Terrain::createGraphicsPipeline () {
	VkDescriptorSetLayoutBinding objdslbindings[2] {{0,
													 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
													 1,
													 VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
													 VK_SHADER_STAGE_FRAGMENT_BIT,
													 nullptr
													}, {
															1,
															VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
															1,
															VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
															VK_SHADER_STAGE_FRAGMENT_BIT,
															nullptr
													}};
	VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {{
															   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
															   nullptr,
															   0,
															   0,
															   nullptr
													   }, {
															   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
															   nullptr,
															   0,
															   2,
															   &objdslbindings[0]
													   }};

	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pii.shaderfilepathprefix = "voxeltroubleshooting";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)};

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.voxeltroubleshootingpipeline, pii);
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
	vkCmdPushConstants(
			cb,
			data.pipeline.pipelinelayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(TerrainGenPushConstants),
			data.pushconstantdata);
	TerrainGenPushConstants* pctemp = reinterpret_cast<TerrainGenPushConstants*>(data.pushconstantdata);
	if (pctemp->phase == 0) vkCmdDispatch(cb, pctemp->numleaves, 1, 1);
	else if (pctemp->phase == 1) vkCmdDispatch(cb, pctemp->numleaves / 8, 1, 1);
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

void* Terrain::getNodeHeapPtr () {
	void* ptr;
	vkMapMemory(
			GraphicsHandler::vulkaninfo.logicaldevice,
			tsbmemory,
			0u,
			numnodes * NODE_SIZE,
			0,
			&ptr);
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, tsbmemory);
	return ptr;
}

void Terrain::updateNumLeaves () {
	Node* root = reinterpret_cast<Node*>(getNodeHeapPtr());
	if (root->childidx == 0) return;
	numleaves = root->childidx;
	GraphicsHandler::vulkaninfo.terraingenpushconstants.numleaves = numleaves;
	std::cout << numleaves << std::endl;
}

glm::vec3 childDirection (uint8_t childidx) {
	if (childidx == 0) return glm::vec3(1., 1., 1.);
	if (childidx == 1) return glm::vec3(-1., 1., 1.);
	if (childidx == 2) return glm::vec3(-1., -1., 1.);
	if (childidx == 3) return glm::vec3(1., -1., 1.);
	if (childidx == 4) return glm::vec3(1., 1., -1.);
	if (childidx == 5) return glm::vec3(-1., 1., -1.);
	if (childidx == 6) return glm::vec3(-1., -1., -1.);
	return glm::vec3(1., -1., -1.);
}

glm::vec3 Terrain::getLeafPos (uint32_t leafidx, uint32_t& nodeidx, Node* heap) {
	uint32_t tempptr = 0u, depth = 0u;
	glm::vec3 position = glm::vec3(0.);
	// go to bottom leftmost
	while (heap[tempptr].children[0] != -1u) {
		tempptr = heap[tempptr].children[0];
		depth++;
		position += glm::vec3(1.) * pow(0.5, depth);
	}
	uint8_t childidx = 0u;
	for (uint32_t i = 0u; i < leafidx; i++) {
		// if we can continue on the same level, do so
		if (childidx != 7) {
			position -= childDirection(childidx) * pow(0.5, depth);
			childidx++;
			position += childDirection(childidx) * pow(0.5, depth);
			tempptr = heap[heap[tempptr].parent].children[childidx];
			// go BLM just in case
			while (heap[tempptr].children[0] != -1u) {
				tempptr = heap[tempptr].children[0];
				depth++;
				position += glm::vec3(1.) * pow(0.5, depth);
				childidx = 0u;
			}

		}
			// if we have reached the end of the level
		else {
			// step up until we find a sibling to the right
			while (childidx == 7) {
				position -= childDirection(childidx) * pow(0.5, depth);
				depth--;
				tempptr = heap[tempptr].parent;
				childidx = heap[tempptr].childidx;
			}
			// go to that sibling
			position -= childDirection(childidx) * pow(0.5, depth);
			childidx++;
			position += childDirection(childidx) * pow(0.5, depth);
			tempptr = heap[heap[tempptr].parent].children[childidx];
			// go BLM
			while (heap[tempptr].children[0] != -1u) {
				tempptr = heap[tempptr].children[0];
				depth++;
				position += glm::vec3(1.) * pow(0.5, depth);
				childidx = 0u;
			}
		}
	}
	nodeidx = tempptr;
	return position;
}

void Terrain::subdivide (uint32_t idx, Node* heap) {
	uint32_t freeptr = heap[0].parent, nextfree = heap[freeptr].parent;
	for (uint8_t i = 0; i < 8; i++) {
		heap[idx].children[i] = freeptr + i;
		heap[freeptr + i].parent = idx;
		heap[freeptr + i].childidx = i;
		heap[freeptr + i].voxel = 0u;
		heap[freeptr + i].children[0] = -1u; // necessary??
		heap[freeptr + i].children[1] = 0u;
	}
	numleaves += 7;
	// updating first free record, and updating that block to have no previous and ensure it is marked free
	heap[0].parent = nextfree;
	heap[nextfree].childidx = -1u;
	heap[nextfree].voxel = -1u; // necessary??
}

void Terrain::collapse (uint32_t parentidx, Node* heap) {
	uint32_t freeptr = heap[0].parent;
	while (freeptr < heap[parentidx].children[0]) {
		freeptr = heap[freeptr].parent;
	}
	numleaves -= 7;
	heap[heap[parentidx].children[0]].parent = heap[freeptr].parent;
	heap[heap[parentidx].children[0]].childidx = freeptr;
	heap[heap[parentidx].children[0]].voxel = -1u;
	// set relevant collapse flags for GPU to take over
	heap[heap[parentidx].children[0]].children[0] = -1u; // necessary??
	heap[heap[parentidx].children[0]].children[1] = 1u;
}

void Terrain::updateVoxels (glm::vec3 camerapos) {
	void* heap;
	vkMapMemory(
			GraphicsHandler::vulkaninfo.logicaldevice,
			tsbmemory,
			0u,
			numnodes * NODE_SIZE,
			0,
			&heap);
	Node* nodeheap = reinterpret_cast<Node*>(heap);

	uint32_t dist, nodeidx, depth, tempptr;
	// std::cout << numleaves << std::endl;
	for (uint32_t leafidx = 0; leafidx < numleaves; leafidx++) {
		dist = glm::distance(camerapos, getLeafPos(leafidx, nodeidx, nodeheap));
		depth = 0u;
		tempptr = nodeidx;
		while (tempptr != 0u) {
			depth++;
			tempptr = nodeheap[tempptr].parent;
		}
		if (dist > 5.) nodeheap[nodeidx].voxel = 0;
		if (dist < 5.) {
			nodeheap[nodeidx].voxel = 1;
			if (depth < 4) subdivide(nodeidx, nodeheap);
		}
	}
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, tsbmemory);
}
