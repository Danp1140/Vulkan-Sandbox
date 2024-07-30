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

void TextureMonitor::updateDescriptorSet (const TextureInfo& t) {
	GraphicsHandler::updateDescriptorSet(
		ds,
		{0},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		{t.getDescriptorImageInfo()}, {});
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
