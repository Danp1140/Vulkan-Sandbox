#include "Compositing.h"

TextureInfo CompositingOp::scratchbuffer = {};
TextureInfo CompositingOp::scratchdepthbuffer = {};
uint8_t CompositingOp::numcompops = 0;

CompositingOp::CompositingOp () {
	VK_INIT_GUARD
	if (numcompops == 0) {
		std::cout << "first comp" << std::endl;
		scratchbuffer.sampler = GraphicsHandler::linearminmagsampler;
		scratchbuffer.format = SWAPCHAIN_IMAGE_FORMAT;
		scratchbuffer.resolution = GraphicsHandler::vulkaninfo.swapchainextent;
		scratchbuffer.memoryprops = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		scratchbuffer.type = TEXTURE_TYPE_SSRR_BUFFER;
		GraphicsHandler::VKHelperInitTexture(scratchbuffer);
		scratchdepthbuffer.sampler = GraphicsHandler::genericsampler;
		scratchdepthbuffer.format = COMPOSITING_SCRATCH_DEPTH_BUFFER_FORMAT;
		scratchdepthbuffer.resolution = GraphicsHandler::vulkaninfo.swapchainextent;
		scratchdepthbuffer.memoryprops = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		scratchdepthbuffer.type = TEXTURE_TYPE_SSRR_BUFFER;
		GraphicsHandler::VKHelperInitTexture(scratchdepthbuffer);
	}
	numcompops++;
}

CompositingOp::~CompositingOp () {
	VK_INIT_GUARD
	if (GraphicsHandler::vulkaninfo.logicaldevice == VK_NULL_HANDLE) return;
	numcompops--;
	if (numcompops == 0) {
		std::cout << "last comp" << std::endl;
		GraphicsHandler::destroyTexture(scratchbuffer);
		GraphicsHandler::destroyTexture(scratchdepthbuffer);
	}
}

VkDependencyInfoKHR SSRR::getPreCopyDependency () {
	VkImageMemoryBarrier2KHR* imbs = new VkImageMemoryBarrier2KHR[2];
	imbs[0] = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		scratchbuffer.image,
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	imbs[1] = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		scratchdepthbuffer.image,
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	};
	return {
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
		nullptr,
		0,
		0, nullptr,
		0, nullptr,
		2, imbs
	};
}

VkDependencyInfoKHR SSRR::getPostCopyDependency () {
	VkImageMemoryBarrier2KHR* imbs = new VkImageMemoryBarrier2KHR[2];
	imbs[0] = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		scratchbuffer.image,
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	imbs[1] = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		scratchdepthbuffer.image,
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	};
	return {
		VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
		nullptr,
		0,
		0, nullptr,
		0, nullptr,
		2, imbs
	};

}

void SSRR::recordCopy (VkCommandBuffer& cb) {
	VkImageCopy2 imgcpy {
		VK_STRUCTURE_TYPE_IMAGE_COPY_2,
		nullptr,
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0},
		{scratchbuffer.resolution.width, scratchbuffer.resolution.height, 1}
	};
	VkCopyImageInfo2 cpyinfo {
		VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
		nullptr,
		GraphicsHandler::vulkaninfo.swapchainimages[GraphicsHandler::swapchainimageindex],
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		scratchbuffer.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &imgcpy
	};
	GraphicsHandler::recordImgCpy({}, cpyinfo, cb);
	VkImageCopy2 depthimgcpy {
		VK_STRUCTURE_TYPE_IMAGE_COPY_2,
		nullptr,
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1}, {0, 0, 0},
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1}, {0, 0, 0},
		{scratchdepthbuffer.resolution.width, scratchdepthbuffer.resolution.height, 1}
	};
	VkCopyImageInfo2 depthcpyinfo {
		VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
		nullptr,
		GraphicsHandler::vulkaninfo.depthbuffer.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // i'm confused here, check renderpass, may need layout transition
		scratchdepthbuffer.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &depthimgcpy
	};
	GraphicsHandler::recordImgCpy({}, depthcpyinfo, cb);
}

uint8_t PostProcOp::numpostprocops = 0;
PipelineInfo PostProcOp::graphicspipeline = {};
PipelineInfo PostProcOp::computepipeline = {};
VkDescriptorSet PostProcOp::ds = VK_NULL_HANDLE;

PostProcOp::PostProcOp () : CompositingOp () {
	VK_INIT_GUARD
	if (numpostprocops == 0) {
		createGraphicsPipeline();
		createComputePipeline();
		initDescriptorSets();
		updateDescriptorSets();
	}
	numpostprocops++;
}

PostProcOp::~PostProcOp () {
	VK_INIT_GUARD
	numpostprocops--;
	if (numpostprocops == 0) {
		vkFreeDescriptorSets(
			GraphicsHandler::vulkaninfo.logicaldevice,
			GraphicsHandler::vulkaninfo.descriptorpool,
			1, &ds);
		GraphicsHandler::destroyPipeline(computepipeline);
		GraphicsHandler::destroyPipeline(graphicspipeline);
	}
}

void PostProcOp::createGraphicsPipeline () {
	PipelineInitInfo pii = {};
	VkDescriptorSetLayoutBinding objdslbindings[4] {{
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr
		}, {
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr
		}, {
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr
		}, {
			3,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
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
			4,
			&objdslbindings[0]
	}};
	// note: this shares dslayout w/ compute, and current impl shares ds's too
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	pii.shaderfilepathprefix = "postproc";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_FRAGMENT_BIT, 0u, sizeof(PostProcPushConstants)},

	GraphicsHandler::VKSubInitPipeline(&graphicspipeline, pii);
}

void PostProcOp::createComputePipeline () {
	PipelineInitInfo pii = {};
	VkDescriptorSetLayoutBinding objdslbindings[4] {{
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		}, {
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		}, {
			2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
		}, {
			3,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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
			4,
			&objdslbindings[0]
	}};
	pii.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	pii.shaderfilepathprefix = "postproc";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_COMPUTE_BIT, 0u, sizeof(PostProcPushConstants)};

	GraphicsHandler::VKSubInitPipeline(&computepipeline, pii);
}

void PostProcOp::initDescriptorSets () {
	VkDescriptorSetAllocateInfo allocinfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.descriptorpool,
		1, &computepipeline.objectdsl
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &allocinfo, &ds);
}

void PostProcOp::updateDescriptorSets () {
	// if updating the whole ds for the SCI every frame proves too much, we can either just update the SCI, or
	// have an array of ds's just for this
	std::vector<VkDescriptorImageInfo> diis = {{
		GraphicsHandler::genericsampler, 
		GraphicsHandler::vulkaninfo.swapchainimageviews[GraphicsHandler::swapchainimageindex],
		VK_IMAGE_LAYOUT_GENERAL
	}, {
		GraphicsHandler::genericsampler, 
		scratchbuffer.imageview,
		VK_IMAGE_LAYOUT_GENERAL
	}, {
		scratchbuffer.sampler, 
		GraphicsHandler::vulkaninfo.swapchainimageviews[GraphicsHandler::swapchainimageindex],
		VK_IMAGE_LAYOUT_GENERAL
	}, {
		scratchbuffer.sampler, 
		scratchbuffer.imageview,
		VK_IMAGE_LAYOUT_GENERAL
	}};
	GraphicsHandler::updateDescriptorSet(
		ds,
		{0, 1, 2, 3},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		diis, {{}, {}, {}, {}});
}
