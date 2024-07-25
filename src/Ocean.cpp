//
// Created by Daniel Paavola on 8/5/21.
//

#include "Ocean.h"

Ocean::Ocean (glm::vec3 pos, glm::vec2 b, Mesh* s) : Mesh(pos) {
	bounds = b;
	shore = s;
	float subdivwidth = 1.0 / (float)OCEAN_PRE_TESS_SUBDIV;
	glm::vec2 dbounds = bounds / (float)OCEAN_PRE_TESS_SUBDIV;
	for (uint16_t y = 0; y < OCEAN_PRE_TESS_SUBDIV + 1; y++) {
		for (uint16_t x = 0; x < OCEAN_PRE_TESS_SUBDIV + 1; x++) {
			vertices.push_back({pos + glm::vec3(dbounds.x * x, 0.0, dbounds.y * y), glm::vec3(0.0, 1.0, 0.0),
								glm::vec2(x * subdivwidth, y * subdivwidth)});
		}
	}
	for (uint16_t y = 0; y < OCEAN_PRE_TESS_SUBDIV; y++) {
		for (uint16_t x = 0; x < OCEAN_PRE_TESS_SUBDIV; x++) {
			tris.push_back({});
			tris.back().vertexindices[0] = (OCEAN_PRE_TESS_SUBDIV + 1) * y + x;
			tris.back().vertexindices[1] = (OCEAN_PRE_TESS_SUBDIV + 1) * (y + 1) + x;
			tris.back().vertexindices[2] = (OCEAN_PRE_TESS_SUBDIV + 1) * (y + 1) + x + 1;
			tris.push_back({});
			tris.back().vertexindices[0] = (OCEAN_PRE_TESS_SUBDIV + 1) * (y + 1) + x + 1;
			tris.back().vertexindices[1] = (OCEAN_PRE_TESS_SUBDIV + 1) * y + x + 1;
			tris.back().vertexindices[2] = (OCEAN_PRE_TESS_SUBDIV + 1) * y + x;
		}
	}
	cleanUpVertsAndTris();
	//consider using smaller formats
	GraphicsHandler::VKHelperInitImage(&heightmap,
									   512, 512,
									   VK_FORMAT_R32_SFLOAT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
									   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
									   VK_IMAGE_LAYOUT_GENERAL,
									   VK_IMAGE_VIEW_TYPE_2D);
	GraphicsHandler::VKHelperInitImage(&velocitymap,
									   512,
									   512,
									   VK_FORMAT_R32G32B32A32_SFLOAT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
									   VK_IMAGE_USAGE_STORAGE_BIT,
									   VK_IMAGE_LAYOUT_GENERAL,
									   VK_IMAGE_VIEW_TYPE_2D);

	glm::vec4* datatemp = (glm::vec4*)malloc(512 * 512 * sizeof(glm::vec4));
	memset((void*)datatemp, 0, 512 * 512 * sizeof(glm::vec4));
	GraphicsHandler::VKHelperInitTexture(&normalmap,
										 2048, 0,
										 VK_FORMAT_R32G32B32A32_SFLOAT,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										 TEXTURE_TYPE_NORMAL,
										 VK_IMAGE_VIEW_TYPE_2D,
										 GraphicsHandler::genericsampler);
	free(datatemp);
//	TextureHandler::generateTextures({normalmap}, TextureHandler::oceanTexGenSet);
	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(
			vertices,
			tris,
			&vertexbuffer,
			&vertexbuffermemory,
			&indexbuffer,
			&indexbuffermemory);
	initComputeCommandBuffers();
	initDescriptorSets();
}

// this was originally intended to help calculate waves, we'll see if we ever get around to it lol
void Ocean::renderDepthMap (Mesh* seabed) {
	vkQueueWaitIdle(GraphicsHandler::vulkaninfo.graphicsqueue);

	GraphicsHandler::VKHelperInitTexture(
			&seabeddepthmap,
			256, 0,
			VK_FORMAT_D32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_SHADOWMAP,
			VK_IMAGE_VIEW_TYPE_2D,
			GraphicsHandler::depthsampler);

	VkFramebuffer framebuffertemp;
	VkFramebufferCreateInfo fbci {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			GraphicsHandler::vulkaninfo.templateshadowrenderpass,
			1,
			&seabeddepthmap.imageview,
			256, 256, 1 //can crank this, but dont need to, sampling is limited accuracy
	};
	vkCreateFramebuffer(
			GraphicsHandler::vulkaninfo.logicaldevice,
			&fbci,
			nullptr,
			&framebuffertemp);

	glm::mat4 projtemp = glm::ortho<float>(-10., 10., -10., 10., 0., 10.);
	projtemp[1][1] *= -1;
	ShadowmapPushConstants temppcs {
			projtemp * glm::lookAt(glm::vec3(0., 5., 0.), glm::vec3(0., -1., 0.), glm::vec3(0., 0., 1.))
	};
//	seabed->recordShadowDraw(0, 0,
//							 GraphicsHandler::vulkaninfo.templateshadowrenderpass,
//							 framebuffertemp,
//							 0,
//							 &temppcs);

	VkCommandBufferBeginInfo cmdbuffbi {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.interimcommandbuffer, &cmdbuffbi);
	VkClearValue cleartemp = {0.};
	VkRenderPassBeginInfo renderpassbi {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.templateshadowrenderpass,
			framebuffertemp,
			{{0, 0}, {256, 256}},
			1,
			&cleartemp
	};
	vkCmdBeginRenderPass(GraphicsHandler::vulkaninfo.interimcommandbuffer, &renderpassbi,
						 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
//	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.interimcommandbuffer, 1, &seabed->getShadowCommandBuffers()[0][0]);
	vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.interimcommandbuffer);
	vkEndCommandBuffer(GraphicsHandler::vulkaninfo.interimcommandbuffer);

	VkSubmitInfo subinfo {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&GraphicsHandler::vulkaninfo.interimcommandbuffer,
			0,
			nullptr
	};
	vkQueueSubmit(GraphicsHandler::vulkaninfo.graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(GraphicsHandler::vulkaninfo.graphicsqueue);

//	vkDestroyFramebuffer(GraphicsHandler::vulkaninfo.logicaldevice, framebuffertemp, nullptr);

	GraphicsHandler::VKHelperTransitionImageLayout(seabeddepthmap.image, 1, VK_FORMAT_D32_SFLOAT,
												   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
												   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Ocean::createComputePipeline () {
	VkDescriptorSetLayoutBinding objdslbindings[2] {{
															0,
															VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
															1,
															VK_SHADER_STAGE_COMPUTE_BIT,
															nullptr
													}, {
															1,
															VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
															1,
															VK_SHADER_STAGE_COMPUTE_BIT,
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
	pii.shaderfilepathprefix = "ocean";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)};

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.oceancomputepipeline, pii);
}

void Ocean::createGraphicsPipeline () {
	VkDescriptorSetLayoutBinding objectdslbindings[3] {{
															   0,
															   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
															   1,
															   VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
															   nullptr
													   }, {
															   1,
															   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
															   1,
															   VK_SHADER_STAGE_FRAGMENT_BIT,
															   nullptr
													   }, {
															   2,
															   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
															   1,
															   VK_SHADER_STAGE_FRAGMENT_BIT,
															   nullptr
													   }};
	VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {
			scenedslcreateinfo,
			{
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					nullptr,
					0,
					3, &objectdslbindings[0]
			}};
	VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdesc[3] {{0,
															0,
															VK_FORMAT_R32G32B32_SFLOAT,
															offsetof(Vertex, position)},
														   {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
														   {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}};

	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT
				 | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
				 | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
				 | VK_SHADER_STAGE_FRAGMENT_BIT;
	pii.shaderfilepathprefix = "ocean";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
							 | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
							 | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(OceanPushConstants)},
			pii.vertexinputstatecreateinfo = {
					VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
					nullptr,
					0,
					1,
					&vertinbindingdesc,
					3,
					&vertinattribdesc[0]
			};
	pii.depthtest = true;

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.oceangraphicspipeline, pii);
}

void Ocean::initDescriptorSets () {
	// note that the top part of this init is gonna be redundant with the mesh-side init
	// so this looks like a double-init with the Mesh constructor, which could be causing the issues we're seeing
	VkDescriptorSetLayout dsltemp[GraphicsHandler::vulkaninfo.numswapchainimages];
	for (unsigned char x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++)
		dsltemp[x] = GraphicsHandler::vulkaninfo.oceangraphicspipeline.objectdsl;
	VkDescriptorSetAllocateInfo descriptorsetallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			GraphicsHandler::vulkaninfo.numswapchainimages,
			&dsltemp[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &descriptorsetallocinfo, &descriptorsets[0]);
	computedescriptorsets = new VkDescriptorSet[GraphicsHandler::vulkaninfo.numswapchainimages];
	for (unsigned char x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++)
		dsltemp[x] = GraphicsHandler::vulkaninfo.oceancomputepipeline.objectdsl;
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &descriptorsetallocinfo,
							 &computedescriptorsets[0]);
}

void Ocean::initComputeCommandBuffers () {
	computecommandbuffers = new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo cmdbufallocinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.commandpool,
			VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			MAX_FRAMES_IN_FLIGHT
	};
	vkAllocateCommandBuffers(GraphicsHandler::vulkaninfo.logicaldevice, &cmdbufallocinfo, computecommandbuffers);
}

void Ocean::rewriteTextureDescriptorSets () {
	for (unsigned char x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++) {
		VkDescriptorImageInfo imginfo = {GraphicsHandler::genericsampler, heightmap.imageview, VK_IMAGE_LAYOUT_GENERAL};
		VkWriteDescriptorSet writedescset {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				descriptorsets[x],
				0,
				0,
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imginfo,
				nullptr,
				nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

		imginfo.sampler = VK_NULL_HANDLE;
		writedescset.dstSet = computedescriptorsets[x];
		writedescset.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

//		imginfo=velocitymap.getDescriptorImageInfo();
		imginfo = {VK_NULL_HANDLE, velocitymap.imageview, VK_IMAGE_LAYOUT_GENERAL};
		writedescset.dstBinding = 1;
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

		imginfo = normalmap.getDescriptorImageInfo();
		writedescset.dstSet = descriptorsets[x];
		writedescset.dstBinding = 1;
		writedescset.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//		writedescset.pImageInfo=&imginfo;
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

		imginfo = {GraphicsHandler::linearminmagsampler,
				   GraphicsHandler::vulkaninfo.scratchbuffer.imageview,
				   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		writedescset.dstBinding = 2;
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);
		// could p consolidate these calls lol

		imginfo.imageView = GraphicsHandler::vulkaninfo.scratchdepthbuffer.imageview;
		writedescset.dstBinding = 3;
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

//		imginfo={GraphicsHandler::genericsampler, GraphicsHandler::vulkaninfo.swapchainimageviews[x], VK_IMAGE_LAYOUT_GENERAL};
//		writedescset.dstBinding=2;
		//figure this out later
//		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

//		imginfo=environmentmap->getDescriptorImageInfo();
//		writedescset.dst
	}
}

// TODO: remove old recordDraw & recordCompute functions
void Ocean::recordDraw (uint8_t fifindex, uint8_t sciindex, VkDescriptorSet* sceneds) {
//	VkCommandBufferInheritanceInfo cmdbufinherinfo {
//			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
//			nullptr,
//			GraphicsHandler::vulkaninfo.primaryrenderpass,
//			0,
//			GraphicsHandler::vulkaninfo.primaryframebuffers[sciindex],
//			VK_FALSE,
//			0,
//			0
//	};
//	VkCommandBufferBeginInfo cmdbufbegininfo {
//			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//			nullptr,
//			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
//			&cmdbufinherinfo
//	};
//	vkBeginCommandBuffer(commandbuffers[fifindex], &cmdbufbegininfo);
//
//	vkCmdBindPipeline(
//			commandbuffers[fifindex],
//			VK_PIPELINE_BIND_POINT_GRAPHICS,
//			GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipeline);
//	vkCmdPushConstants(
//			commandbuffers[fifindex],
//			GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipelinelayout,
//			VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
//			| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
//			| VK_SHADER_STAGE_FRAGMENT_BIT,
//			0,
//			sizeof(OceanPushConstants),
//			&GraphicsHandler::vulkaninfo.oceanpushconstants);
//	vkCmdBindDescriptorSets(
//			commandbuffers[fifindex],
//			VK_PIPELINE_BIND_POINT_GRAPHICS,
//			GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipelinelayout,
//			0,
//			1,
//			&sceneds[0],
//			0, nullptr);
//	vkCmdBindDescriptorSets(
//			commandbuffers[fifindex],
//			VK_PIPELINE_BIND_POINT_GRAPHICS,
//			GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipelinelayout,
//			1,
//			1,
//			&descriptorsets[0],
//			0, nullptr);
//	VkDeviceSize offsettemp = 0u;
//	vkCmdBindVertexBuffers(
//			commandbuffers[fifindex],
//			0,
//			1,
//			&vertexbuffer,
//			&offsettemp);
//	vkCmdBindIndexBuffer(
//			commandbuffers[fifindex],
//			indexbuffer,
//			0,
//			VK_INDEX_TYPE_UINT16);
//	vkCmdDrawIndexed(
//			commandbuffers[fifindex],
//			tris.size() * 3,
//			1,
//			0,
//			0,
//			0);
//	vkEndCommandBuffer(commandbuffers[fifindex]);
}

void Ocean::recordDraw (cbRecData data, VkCommandBuffer& cb) {
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

	vkCmdBindPipeline(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipeline);
	vkCmdPushConstants(
			cb,
			data.pipeline.pipelinelayout,
			VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
			| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
			| VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(OceanPushConstants),
			data.pushconstantdata);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipelinelayout,
			0,
			1, &data.scenedescriptorset,
			0, nullptr);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipelinelayout,
			1,
			1, &data.descriptorset,
			0, nullptr);
	VkDeviceSize offsettemp = 0u;
	vkCmdBindVertexBuffers(
			cb,
			0,
			1,
			&data.vertexbuffer,
			&offsettemp);
	vkCmdBindIndexBuffer(
			cb,
			data.indexbuffer,
			0,
			VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(
			cb,
			data.numtris * 3,
			1,
			0,
			0,
			0);
	vkEndCommandBuffer(cb);
}

void Ocean::recordCompute (uint8_t fifindex) {
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
	vkBeginCommandBuffer(computecommandbuffers[fifindex], &cmdbufbegininfo);
	//should probably have some barriers around here
	vkCmdBindPipeline(
			computecommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			GraphicsHandler::vulkaninfo.oceancomputepipeline.pipeline);
	float timetemp = (float)glfwGetTime();
	vkCmdPushConstants(
			computecommandbuffers[fifindex],
			GraphicsHandler::vulkaninfo.oceancomputepipeline.pipelinelayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(float),
			(void*)(&timetemp));
	vkCmdBindDescriptorSets(
			computecommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			GraphicsHandler::vulkaninfo.oceancomputepipeline.pipelinelayout,
			0,
			1,
			&computedescriptorsets[0],
			0, nullptr);
	vkCmdDispatch(computecommandbuffers[fifindex], 512, 512, 1);
//	VkImageMemoryBarrier imgmembarr{
//		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//		nullptr,
//		VK_ACCESS_SHADER_WRITE_BIT,
//		VK_ACCESS_SHADER_READ_BIT,
//		VK_IMAGE_LAYOUT_GENERAL,
//		VK_IMAGE_LAYOUT_GENERAL,
//		VK_QUEUE_FAMILY_IGNORED,
//		VK_QUEUE_FAMILY_IGNORED,
//		ocean->getHeightMapPtr()->image,
//		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
//	};
//	//idk if this is doing what it needs to...
//	vkCmdPipelineBarrier(GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
//					     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
//					     VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
//					     0,
//					     0, nullptr,
//					     0, nullptr,
//					     1, &imgmembarr);
	vkEndCommandBuffer(computecommandbuffers[fifindex]);
}

void Ocean::recordCompute (cbRecData data, VkCommandBuffer& cb) {
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
	//should probably have some barriers around here
	vkCmdBindPipeline(
			cb,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			data.pipeline.pipeline);
	float timetemp = (float)glfwGetTime();
	vkCmdPushConstants(
			cb,
			GraphicsHandler::vulkaninfo.oceancomputepipeline.pipelinelayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(float), data.pushconstantdata);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			data.pipeline.pipelinelayout,
			0,
			1, &data.descriptorset,
			0, nullptr);
	vkCmdDispatch(cb, 512, 512, 1);
//	VkImageMemoryBarrier imgmembarr{
//		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//		nullptr,
//		VK_ACCESS_SHADER_WRITE_BIT,
//		VK_ACCESS_SHADER_READ_BIT,
//		VK_IMAGE_LAYOUT_GENERAL,
//		VK_IMAGE_LAYOUT_GENERAL,
//		VK_QUEUE_FAMILY_IGNORED,
//		VK_QUEUE_FAMILY_IGNORED,
//		ocean->getHeightMapPtr()->image,
//		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
//	};
//	//idk if this is doing what it needs to...
//	vkCmdPipelineBarrier(GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
//					     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
//					     VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
//					     0,
//					     0, nullptr,
//					     0, nullptr,
//					     1, &imgmembarr);
	vkEndCommandBuffer(cb);
}

void Ocean::rewriteDescriptorSet (uint32_t index) {
	VkDescriptorImageInfo imginfo = heightmap.getDescriptorImageInfo();
	VkWriteDescriptorSet write {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorsets[index],
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imginfo,
			nullptr,
			nullptr,
	};
	vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &write, 0, nullptr);

	imginfo = normalmap.getDescriptorImageInfo();
	write.dstBinding = 1;
	write.pImageInfo = &imginfo;
	//could p write both at once
	vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &write, 0, nullptr);
}
