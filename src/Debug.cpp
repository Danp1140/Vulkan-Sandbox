#include "Debug.h"

PipelineInfo TextureMonitor::graphicspipeline = {};

TextureMonitor::TextureMonitor () : ds(VK_NULL_HANDLE) {
	VK_INIT_GUARD
	VkDescriptorSetAllocateInfo dsallocinfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.descriptorpool,
		1, &graphicspipeline.objectdsl
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &dsallocinfo, &ds);
}

TextureMonitor::~TextureMonitor () {
	if (ds != VK_NULL_HANDLE) { 
		vkFreeDescriptorSets(
			GraphicsHandler::vulkaninfo.logicaldevice, 
			GraphicsHandler::vulkaninfo.descriptorpool, 
			1, &ds);
	}
}

void TextureMonitor::updateDescriptorSet (const VkDescriptorImageInfo& dii) {
	GraphicsHandler::updateDescriptorSet(
		ds,
		{0},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		{dii}, {});
}

void TextureMonitor::init() {
	createPipeline();
}

void TextureMonitor::terminate() {
	GraphicsHandler::destroyPipeline(graphicspipeline);
}

void TextureMonitor::createPipeline () {
	PipelineInitInfo pii = {};
	VkDescriptorSetLayoutBinding descriptorsetlayoutbinding {
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfos[2] {{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			0,
			nullptr
		}, {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1,
			&descriptorsetlayoutbinding
	}};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	pii.descsetlayoutcreateinfos = &descriptorsetlayoutcreateinfos[0];
	pii.shaderfilepathprefix = "texmon";
	pii.renderpass = GraphicsHandler::vulkaninfo.compositingrenderpass;

	GraphicsHandler::VKSubInitPipeline(&graphicspipeline, pii);
}

void TextureMonitor::recordDraw (cbRecData data, VkCommandBuffer& cb) {
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
	vkCmdBindDescriptorSets(
		cb,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		data.pipeline.pipelinelayout,
		0,
		1, &data.descriptorset,
		0, nullptr);
	vkCmdDraw(cb, 6, 1, 0, 0);
	vkEndCommandBuffer(cb);
}

PipelineInfo Lines::graphicspipeline = {};

Lines::Lines () : vertexbuffer({}), pushconstants({}) {
	std::vector<Vertex> troubleshootinglinespoints;
//	std::vector<glm::vec3> controlpoints {
//			glm::vec3(11., 0., 11.),
//			glm::vec3(10., 0., -8.),
//			glm::vec3(4., 0., -6.),
//			glm::vec3(4., 0., 4.),
//			glm::vec3(-4., 0., 6.),
//			glm::vec3(-6., 0., 4.),
//			glm::vec3(-10., 0., -4),
//			glm::vec3(-11., 0., 4.)
//	};
//	glm::vec3 splinePos;
//	for (uint8_t p1 = 1; p1 + 2 < controlpoints.size(); p1++) {
//		for (uint8_t x = 0; x < 25; x++) {
//			splinePos = PhysicsHandler::catmullRomSplineAtMatrified(
//					controlpoints,
//					p1,
//					0.5,
//					float_t(x) / 24.f);
//			troubleshootinglinespoints.push_back({
//														 splinePos,
//														 glm::vec3(0., 0., 0.),
//														 glm::vec2(0., 0.)
//												 });
//			troubleshootinglinespoints.push_back({
//														 splinePos,
//														 glm::vec3(0., 0., 0.),
//														 glm::vec2(0., 0.)
//												 });
//
//		}
//	}
//	troubleshootinglinespoints.erase(troubleshootinglinespoints.begin());
//	troubleshootinglinespoints.erase(troubleshootinglinespoints.end() - 1);

	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(1, 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 1, 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 1), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});

	GraphicsHandler::VKHelperInitVertexBuffer(
			troubleshootinglinespoints,
			&vertexbuffer.buffer,
			&vertexbuffer.memory);
	vertexbuffer.numelems = troubleshootinglinespoints.size();
}

Lines::~Lines () {

}

void Lines::init () {
	createPipeline();
}

void Lines::terminate () {
	GraphicsHandler::destroyPipeline(graphicspipeline);
}

void Lines::recordDraw (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cbii {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		nullptr,
		data.renderpass,
		0,
		data.framebuffer,
		VK_FALSE,
		0,
		0
	};
	VkCommandBufferBeginInfo cbbi {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		&cbii
	};
	vkBeginCommandBuffer(cb, &cbbi);
	vkCmdBindPipeline(
		cb,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		data.pipeline.pipeline);
	vkCmdPushConstants(
		cb,
		data.pipeline.pipelinelayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(LinesPushConstants),
		data.pushconstantdata);
	VkDeviceSize offsettemp = 0u;
	vkCmdBindVertexBuffers(
		cb,
		0,
		1,
		&data.vertexbuffer,
		&offsettemp);
	vkCmdDraw(
		cb,
		data.numtris, // yeah yeah theyre not triangles, it means num points 
		1,
		0,
		0);
	vkEndCommandBuffer(cb);

}

void Lines::createPipeline () {
	PipelineInitInfo pii = {};
	VkVertexInputBindingDescription vibd {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription viad {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
	pii.vertexinputstatecreateinfo = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		&vibd,
		1,
		&viad
	};
	VkDescriptorSetLayoutCreateInfo dslci[2] {
		scenedslcreateinfo, {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr
	}};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pii.shaderfilepathprefix = "line";
	pii.pushconstantrange = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(LinesPushConstants)};
	pii.descsetlayoutcreateinfos = &dslci[0];
	pii.topo = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	GraphicsHandler::VKSubInitPipeline(&graphicspipeline, pii);
}
