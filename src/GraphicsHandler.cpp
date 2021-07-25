//
// Created by Daniel Paavola on 5/30/21.
//

#include "GraphicsHandler.h"

VulkanInfo GraphicsHandler::vulkaninfo{};
ChangeFlag*GraphicsHandler::changeflags=nullptr;

GraphicsHandler::GraphicsHandler(){}

GraphicsHandler::~GraphicsHandler(){
	vkWaitForFences(vulkaninfo.logicaldevice, MAX_FRAMES_IN_FLIGHT, &vulkaninfo.frameinflightfences[0], VK_TRUE, UINT64_MAX);
	vkQueueWaitIdle(vulkaninfo.graphicsqueue);
	vkQueueWaitIdle(vulkaninfo.presentationqueue);
	vkDeviceWaitIdle(vulkaninfo.logicaldevice);

	for(int x=0;x<MAX_FRAMES_IN_FLIGHT;x++){
		vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.renderfinishedsemaphores[x], nullptr);
		vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.imageavailablesemaphores[x], nullptr);
		vkDestroyFence(vulkaninfo.logicaldevice, vulkaninfo.frameinflightfences[x], nullptr);
	}
	vkDestroyDescriptorSetLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.meshdescriptorsetlayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.scenedescriptorsetlayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkaninfo.logicaldevice, vulkaninfo.textdescriptorsetlayout, nullptr);
	vkDestroyDescriptorPool(vulkaninfo.logicaldevice, vulkaninfo.descriptorpool, nullptr);
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, 1, &(vulkaninfo.interimcommandbuffer));
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, vulkaninfo.numswapchainimages, &vulkaninfo.commandbuffers[0]);
	vkDestroyCommandPool(vulkaninfo.logicaldevice, vulkaninfo.commandpool, nullptr);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffers[x], nullptr);
		vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffermemories[x], nullptr);
		vkDestroyFramebuffer(vulkaninfo.logicaldevice, vulkaninfo.framebuffers[x], nullptr);
	}
	vkDestroyShaderModule(vulkaninfo.logicaldevice, vulkaninfo.textgraphicspipeline.vertexshadermodule, nullptr);
	vkDestroyShaderModule(vulkaninfo.logicaldevice, vulkaninfo.textgraphicspipeline.fragmentshadermodule, nullptr);
	vkDestroyShaderModule(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.vertexshadermodule, nullptr);
	vkDestroyShaderModule(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.fragmentshadermodule, nullptr);
	vkDestroyPipeline(vulkaninfo.logicaldevice, vulkaninfo.textgraphicspipeline.pipeline, nullptr);
	vkDestroyPipeline(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipeline, nullptr);
	vkDestroyRenderPass(vulkaninfo.logicaldevice, vulkaninfo.primaryrenderpass, nullptr);
	vkDestroyPipelineLayout(vulkaninfo.logicaldevice, vulkaninfo.textgraphicspipeline.pipelinelayout, nullptr);
	vkDestroyPipelineLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipelinelayout, nullptr);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) vkDestroyImageView(vulkaninfo.logicaldevice, vulkaninfo.swapchainimageviews[x], nullptr);
	vkDestroySwapchainKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, nullptr);
	vkDestroyDevice(vulkaninfo.logicaldevice, nullptr);
	DestroyDebugUtilsMessengerEXT(vulkaninfo.instance, vulkaninfo.debugmessenger, nullptr);
	vkDestroySurfaceKHR(vulkaninfo.instance, vulkaninfo.surface, nullptr);
	vkDestroyInstance(vulkaninfo.instance, nullptr);
	glfwDestroyWindow(vulkaninfo.window);
	glfwTerminate();
	delete[] vulkaninfo.lightuniformbuffers;
	delete[] vulkaninfo.lightuniformbuffermemories;
	delete[] vulkaninfo.commandbuffers;
	delete[] vulkaninfo.framebuffers;
	delete[] vulkaninfo.swapchainimageviews;
	delete[] vulkaninfo.swapchainimages;
}

void GraphicsHandler::VKInit(){
	VKSubInitWindow();
	VKSubInitInstance();
	VKSubInitDebug();
	VKSubInitDevices();
	VKSubInitQueues();
	VKSubInitSwapchain();
	VKSubInitRenderpass();
	VKSubInitCommandPool();
	VKSubInitDepthBuffer();
	VKSubInitFramebuffers();
	VKSubInitSemaphoresAndFences();
}

void GraphicsHandler::VKInitPipelines(uint32_t nummeshes, uint32_t numlights){
	VKSubInitDescriptorPool(nummeshes);
	VKSubInitGraphicsPipeline();
	VKSubInitTextGraphicsPipeline();
}

void GraphicsHandler::VKSubInitWindow(){
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	const GLFWvidmode*mode=glfwGetVideoMode(glfwGetPrimaryMonitor());
	vulkaninfo.width=mode->width; vulkaninfo.height=mode->height;
	vulkaninfo.window=glfwCreateWindow(vulkaninfo.width, vulkaninfo.height, "Vulkan Sandbox", glfwGetPrimaryMonitor(), nullptr);
	glfwGetFramebufferSize(vulkaninfo.window, &vulkaninfo.horizontalres, &vulkaninfo.verticalres);
	glfwMakeContextCurrent(vulkaninfo.window);
	glfwSetInputMode(vulkaninfo.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GraphicsHandler::VKSubInitInstance(){
	VkApplicationInfo appinfo{
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,
			"Vulkan Sandbox",
			VK_MAKE_VERSION(1, 0, 0),
			"Jet",
			VK_MAKE_VERSION(1, 0, 0),
			VK_MAKE_API_VERSION(1, 1, 0, 0)
	};

	const char*layernames[1]={"VK_LAYER_KHRONOS_validation"};

	uint32_t instanceextcount=-1u;
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceextcount, nullptr);
	VkExtensionProperties instanceextprops[instanceextcount];
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceextcount, &instanceextprops[0]);
	char*instanceextnames[instanceextcount];
	for(uint32_t x=0;x<instanceextcount;x++) instanceextnames[x]=instanceextprops[x].extensionName;

	VkInstanceCreateInfo instancecreateinfo{
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			nullptr,
			0,
			&appinfo,
			1,
			&layernames[0],
			instanceextcount,
			&instanceextnames[0]
	};
	VkValidationFeaturesEXT validationfeatures{
			VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
			instancecreateinfo.pNext,
			0,
			nullptr,
			0,
			nullptr
	};
	instancecreateinfo.pNext=&validationfeatures;
	vkCreateInstance(&instancecreateinfo, nullptr, &vulkaninfo.instance);
	glfwCreateWindowSurface(vulkaninfo.instance, vulkaninfo.window, nullptr, &vulkaninfo.surface);
}

void GraphicsHandler::VKSubInitDebug(){
	VkDebugUtilsMessengerCreateInfoEXT messengercreateinfo{
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		nullptr,
		0,
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		debugCallback,
		nullptr
	};
	CreateDebugUtilsMessengerEXT(vulkaninfo.instance, &messengercreateinfo, nullptr, &vulkaninfo.debugmessenger);
}

void GraphicsHandler::VKSubInitDevices(){
	uint32_t physicaldevicecount=1, queuefamilycount=-1u, deviceextcount=-1u;
	vkEnumeratePhysicalDevices(vulkaninfo.instance, &physicaldevicecount, &vulkaninfo.physicaldevice);
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, nullptr);
	VkQueueFamilyProperties queuefamilyprops[queuefamilycount];
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, &queuefamilyprops[0]);

//	vulkaninfo.queuefamilyindices=new uint32_t[2];
	vulkaninfo.graphicsqueuefamilyindex=-1u; vulkaninfo.presentqueuefamilyindex=-1u; vulkaninfo.transferqueuefamilyindex=-1u;
	VkBool32 supportssurfacepresentation;
	for(uint32_t x=0;x<queuefamilycount;x++){
		if((queuefamilyprops[x].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&vulkaninfo.graphicsqueuefamilyindex==-1u) vulkaninfo.graphicsqueuefamilyindex=x;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkaninfo.physicaldevice, x, vulkaninfo.surface, &supportssurfacepresentation);
		if(supportssurfacepresentation&&vulkaninfo.presentqueuefamilyindex==-1u) vulkaninfo.presentqueuefamilyindex=x;
		if((queuefamilyprops[x].queueFlags&VK_QUEUE_TRANSFER_BIT)&&vulkaninfo.transferqueuefamilyindex==-1u) vulkaninfo.transferqueuefamilyindex=x;
	}
	std::vector<VkDeviceQueueCreateInfo> queuecreateinfos;
	float queuepriority=1.0f;
	queuecreateinfos.push_back({
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		vulkaninfo.graphicsqueuefamilyindex,
		1,
		&queuepriority
	});
	if(vulkaninfo.graphicsqueuefamilyindex!=vulkaninfo.presentqueuefamilyindex){
		queuecreateinfos.push_back({
	        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	        nullptr,
	        0,
	        vulkaninfo.presentqueuefamilyindex,
	        1,
	        &queuepriority
		});
	}
	if(vulkaninfo.graphicsqueuefamilyindex!=vulkaninfo.transferqueuefamilyindex&&vulkaninfo.presentqueuefamilyindex!=vulkaninfo.transferqueuefamilyindex){
		queuecreateinfos.push_back({
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            vulkaninfo.transferqueuefamilyindex,
            1,
            &queuepriority
		});
	}
	VkDeviceQueueCreateInfo devicequeuecreateinfos[2]{{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		vulkaninfo.graphicsqueuefamilyindex,
		1,
		&queuepriority
	}, {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		vulkaninfo.presentqueuefamilyindex,
		1,
		&queuepriority
	}};
	vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextcount, nullptr);
	VkExtensionProperties deviceextprops[deviceextcount];
	vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextcount, &deviceextprops[0]);
	std::vector<const char*> deviceextnames;
	for(uint32_t x=0;x<deviceextcount;x++){
//		std::cout<<deviceextprops[x].extensionName<<std::endl;
		if(strcmp(deviceextprops[x].extensionName, "VK_KHR_portability_subset")) deviceextnames.push_back("VK_KHR_portability_subset");
		if(strcmp(deviceextprops[x].extensionName, "VK_KHR_swapchain")) deviceextnames.push_back("VK_KHR_swapchain");
	}
	VkPhysicalDeviceFeatures physicaldevicefeatures{};
//	physicaldevicefeatures.samplerAnisotropy=VK_TRUE;
	//could include all available, but for now will include none
	VkDeviceCreateInfo devicecreateinfo{
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr,
			0,
			(uint32_t)queuecreateinfos.size(),
			queuecreateinfos.data(),
			0,
			nullptr,
			(uint32_t)deviceextnames.size(),
			deviceextnames.data(),
			&physicaldevicefeatures
	};
	vkCreateDevice(vulkaninfo.physicaldevice, &devicecreateinfo, nullptr, &vulkaninfo.logicaldevice);
}

void GraphicsHandler::VKSubInitQueues(){
	vkGetDeviceQueue(vulkaninfo.logicaldevice, vulkaninfo.graphicsqueuefamilyindex, 0, &vulkaninfo.graphicsqueue);
	vkGetDeviceQueue(vulkaninfo.logicaldevice, vulkaninfo.presentqueuefamilyindex, 0, &vulkaninfo.presentationqueue);
}

void GraphicsHandler::VKSubInitSwapchain(){
	VkSurfaceCapabilitiesKHR surfacecaps{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &surfacecaps);
	vulkaninfo.swapchainextent=surfacecaps.currentExtent;
	vulkaninfo.numswapchainimages=surfacecaps.minImageCount;
	bool queuefamilyindicesaresame=vulkaninfo.graphicsqueuefamilyindex==vulkaninfo.presentqueuefamilyindex;
	VkSwapchainCreateInfoKHR swapchaincreateinfo{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr,
		0,
		vulkaninfo.surface,
		vulkaninfo.numswapchainimages,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		vulkaninfo.swapchainextent,
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		queuefamilyindicesaresame?VK_SHARING_MODE_EXCLUSIVE:VK_SHARING_MODE_CONCURRENT,
		queuefamilyindicesaresame?0u:2u,
		queuefamilyindicesaresame?nullptr:&vulkaninfo.graphicsqueuefamilyindex,
		surfacecaps.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		VK_TRUE,
		VK_NULL_HANDLE
	};
	vkCreateSwapchainKHR(vulkaninfo.logicaldevice, &swapchaincreateinfo, nullptr, &vulkaninfo.swapchain);
	uint32_t imgcounttemp=-1u;
	//lol i have to call this to get the number once before i can call for values lmao
	vkGetSwapchainImagesKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, &imgcounttemp, nullptr);
	vulkaninfo.swapchainimages=new VkImage[vulkaninfo.numswapchainimages];
	vkGetSwapchainImagesKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, &vulkaninfo.numswapchainimages, &vulkaninfo.swapchainimages[0]);
	vulkaninfo.swapchainimageviews=new VkImageView[vulkaninfo.numswapchainimages];
	changeflags=new ChangeFlag[vulkaninfo.numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		changeflags[x]=NO_CHANGE_FLAG_BIT;
		VkImageViewCreateInfo imageviewcreateinfo{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			vulkaninfo.swapchainimages[x],
			VK_IMAGE_VIEW_TYPE_2D,
			VULKAN_SWAPCHAIN_IMAGE_FORMAT,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};
		vkCreateImageView(vulkaninfo.logicaldevice, &imageviewcreateinfo, nullptr, &vulkaninfo.swapchainimageviews[x]);
	}
}

void GraphicsHandler::VKSubInitRenderpass(){
	VkAttachmentDescription attachmentdescription{
		0,
		VULKAN_SWAPCHAIN_IMAGE_FORMAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
	VkAttachmentDescription depthattachmentdescription{
		0,
		VK_FORMAT_D32_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference attachmentrefs[2]{{
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	}, {
		1,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	}};
	VkSubpassDescription subpassdescription{
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		1,
		&attachmentrefs[0],
		nullptr,
		&attachmentrefs[1],
		0,
		nullptr
	};
	VkSubpassDependency subpassdependency{
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		0
	};
	VkAttachmentDescription attachmentdescriptionstemp[2]{attachmentdescription, depthattachmentdescription};
	VkRenderPassCreateInfo renderpasscreateinfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		2,
		&attachmentdescriptionstemp[0],
		1,
		&subpassdescription,
		1,
		&subpassdependency
	};
	vkCreateRenderPass(vulkaninfo.logicaldevice, &renderpasscreateinfo, nullptr, &vulkaninfo.primaryrenderpass);
}

void GraphicsHandler::VKSubInitDescriptorPool(uint32_t nummeshes){
	VkDescriptorPoolSize descriptorpoolsizes[2]{{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		vulkaninfo.numswapchainimages*VULKAN_MAX_LIGHTS
	}, {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		vulkaninfo.numswapchainimages*(nummeshes+1)
	}};
	VkDescriptorPoolCreateInfo descriptorpoolcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,      //perhaps look into flags
		vulkaninfo.numswapchainimages*(VULKAN_MAX_LIGHTS+nummeshes+1),
		2,
		&descriptorpoolsizes[0]
	};
	vkCreateDescriptorPool(vulkaninfo.logicaldevice, &descriptorpoolcreateinfo, nullptr, &vulkaninfo.descriptorpool);
}

void GraphicsHandler::VKSubInitUniformBuffer(PipelineInfo*pipelineinfo,
											 uint32_t binding,
											 VkDeviceSize elementsize,
											 uint32_t elementcount,
											 VkBuffer*buffers,
											 VkDeviceMemory*memories){
	VkPhysicalDeviceProperties pdprops;
	vkGetPhysicalDeviceProperties(vulkaninfo.physicaldevice, &pdprops);
	VkDeviceSize roundedupsize=elementsize;
	if(elementsize%pdprops.limits.minUniformBufferOffsetAlignment!=0){
		roundedupsize=(1+floor((float)elementsize/(float)pdprops.limits.minUniformBufferOffsetAlignment))*pdprops.limits.minUniformBufferOffsetAlignment;
	}

	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		VkBufferCreateInfo buffcreateinfo{
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				nullptr,
				0,
				roundedupsize*elementcount,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_SHARING_MODE_EXCLUSIVE,
				0,
				nullptr
		};
		vkCreateBuffer(vulkaninfo.logicaldevice, &buffcreateinfo, nullptr, &buffers[x]);
		VkMemoryRequirements memrequirements{};
		vkGetBufferMemoryRequirements(vulkaninfo.logicaldevice, buffers[x], &memrequirements);
		VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
		vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
		uint32_t memindex=-1u;
		for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
			if(memrequirements.memoryTypeBits&(1<<x)
			   &&physicaldevicememprops.memoryTypes[x].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			   &&physicaldevicememprops.memoryTypes[x].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){
				memindex=x;
				break;
			}
		}
		VkMemoryAllocateInfo memallocateinfo{
				VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				nullptr,
				memrequirements.size,
				memindex
		};
		vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, &memories[x]);
		vkBindBufferMemory(vulkaninfo.logicaldevice, buffers[x], memories[x], 0);

		VkDescriptorBufferInfo descriptorbuffinfos[elementcount];
		for(uint32_t y=0;y<elementcount;y++) descriptorbuffinfos[y]={
				buffers[x],
				y*roundedupsize,
				elementsize
		};
		VkWriteDescriptorSet writedescriptorset{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				pipelineinfo->descriptorset[x],
				binding,
				0,
				elementcount,      //i mean, we only /really/ have to update the buffers for active lights
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				nullptr,
				&descriptorbuffinfos[0],
				nullptr
		};
		vkUpdateDescriptorSets(vulkaninfo.logicaldevice, 1, &writedescriptorset, 0, nullptr);
	}
}

void GraphicsHandler::VKSubInitGraphicsPipeline(){
	VkDescriptorSetLayoutBinding scenedslbinding{
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VULKAN_MAX_LIGHTS,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	VkDescriptorSetLayoutCreateInfo scenedslcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&scenedslbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &scenedslcreateinfo, nullptr, &vulkaninfo.primarygraphicspipeline.scenedescriptorsetlayout);
	VkDescriptorSetLayoutBinding meshdslbinding{
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
	};
	VkDescriptorSetLayoutCreateInfo meshdslcreateinfo{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1,
			&meshdslbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &meshdslcreateinfo, nullptr, &vulkaninfo.primarygraphicspipeline.meshdescriptorsetlayout);

	VkDescriptorSetLayout descriptorsetlayoutstemp[vulkaninfo.numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) descriptorsetlayoutstemp[x]=vulkaninfo.primarygraphicspipeline.scenedescriptorsetlayout;
	vulkaninfo.primarygraphicspipeline.descriptorset=new VkDescriptorSet[vulkaninfo.numswapchainimages];
	VkDescriptorSetAllocateInfo descriptorsetallocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		vulkaninfo.descriptorpool,
		vulkaninfo.numswapchainimages,
		&descriptorsetlayoutstemp[0]
	};
	vkAllocateDescriptorSets(vulkaninfo.logicaldevice, &descriptorsetallocinfo, &vulkaninfo.primarygraphicspipeline.descriptorset[0]);

	vulkaninfo.lightuniformbuffers=new VkBuffer[vulkaninfo.numswapchainimages];
	vulkaninfo.lightuniformbuffermemories=new VkDeviceMemory[vulkaninfo.numswapchainimages];
	VKSubInitUniformBuffer(&vulkaninfo.primarygraphicspipeline,
						   0,
						   sizeof(LightUniformBuffer),
						   VULKAN_MAX_LIGHTS,
						   vulkaninfo.lightuniformbuffers,
						   vulkaninfo.lightuniformbuffermemories);

	VkPushConstantRange pushconstantrange{
		VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PrimaryGraphicsPushConstants)
	};
	VkVertexInputBindingDescription vertinbindingdescription{
		0,
		sizeof(Vertex),
		VK_VERTEX_INPUT_RATE_VERTEX
	};
	VkVertexInputAttributeDescription vertinattribdescription[3]{{
		0,
		0,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(Vertex, position)
	}, {
		1,
		0,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(Vertex, normal)
	}, {
		2,
		0,
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(Vertex, uv)
	}};
	VkPipelineVertexInputStateCreateInfo vertinstatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		&vertinbindingdescription,
		3,
		&vertinattribdescription[0]
	};
	//could we just pass vulkaninfo.primarygraphicspipeline to both of these?
	VkPipelineShaderStageCreateInfo vertshadersci, fragshadersci;
	VKSubInitLoadShaders("../resources/shaders/vert.spv",
					     "../resources/shaders/frag.spv",
					     &vulkaninfo.primarygraphicspipeline.vertexshadermodule,
					     &vulkaninfo.primarygraphicspipeline.fragmentshadermodule,
					     &vertshadersci,
					     &fragshadersci);
	VkPipelineShaderStageCreateInfo shaderstagecreateinfostemp[2]{vertshadersci, fragshadersci};
	VkDescriptorSetLayout layoutstemp[2]{vulkaninfo.primarygraphicspipeline.scenedescriptorsetlayout, vulkaninfo.primarygraphicspipeline.meshdescriptorsetlayout};
	VKSubInitPipeline(&vulkaninfo.primarygraphicspipeline,
			          2u,
			          &layoutstemp[0],
			          &pushconstantrange,
			          &vulkaninfo.primaryrenderpass,
			          &shaderstagecreateinfostemp[0],
			          &vertinstatecreateinfo);
}

void GraphicsHandler::VKSubInitTextGraphicsPipeline(){
	VkPipelineShaderStageCreateInfo vertsci, fragsci;
	VKSubInitLoadShaders("../resources/shaders/textvert.spv",
					     "../resources/shaders/textfrag.spv",
					     &vulkaninfo.textgraphicspipeline.vertexshadermodule,
					     &vulkaninfo.textgraphicspipeline.fragmentshadermodule,
					     &vertsci,
					     &fragsci);
	VkPipelineShaderStageCreateInfo shaderscitemps[2]{vertsci, fragsci};
	VkPipelineVertexInputStateCreateInfo vertexinputsci{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr
	};
	VkDescriptorSetLayoutBinding layoutbinding{
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	VkDescriptorSetLayoutCreateInfo layoutcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&layoutbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &layoutcreateinfo, nullptr, &vulkaninfo.textdescriptorsetlayout);
	VkPushConstantRange pushconstantrange{
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(TextPushConstants)
	};
	VKSubInitPipeline(&vulkaninfo.textgraphicspipeline,
			          1,
			          &vulkaninfo.textdescriptorsetlayout,
			          &pushconstantrange,
			          &vulkaninfo.primaryrenderpass,
			          &shaderscitemps[0],
			          &vertexinputsci);
}

void GraphicsHandler::VKSubInitFramebuffers(){
	vulkaninfo.framebuffers=new VkFramebuffer[vulkaninfo.numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		VkImageView attachmentstemp[2]={vulkaninfo.swapchainimageviews[x], vulkaninfo.depthbuffer.imageview};
		VkFramebufferCreateInfo framebuffercreateinfo{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			vulkaninfo.primaryrenderpass,
			2,
			&attachmentstemp[0],
			vulkaninfo.swapchainextent.width,
			vulkaninfo.swapchainextent.height,
			1
		};
		vkCreateFramebuffer(vulkaninfo.logicaldevice, &framebuffercreateinfo, nullptr, &vulkaninfo.framebuffers[x]);
	}
}

void GraphicsHandler::VKSubInitCommandPool(){
	VkCommandPoolCreateInfo cmdpoolcreateinfo{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		vulkaninfo.graphicsqueuefamilyindex
	};
	vkCreateCommandPool(vulkaninfo.logicaldevice, &cmdpoolcreateinfo, nullptr, &vulkaninfo.commandpool);
	VkCommandBufferAllocateInfo cmdbufferallocateinfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		vulkaninfo.commandpool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		vulkaninfo.numswapchainimages
	};
	vulkaninfo.commandbuffers=new VkCommandBuffer[vulkaninfo.numswapchainimages];
	vkAllocateCommandBuffers(vulkaninfo.logicaldevice, &cmdbufferallocateinfo, &vulkaninfo.commandbuffers[0]);
	cmdbufferallocateinfo.commandBufferCount=1;
	vkAllocateCommandBuffers(vulkaninfo.logicaldevice, &cmdbufferallocateinfo, &(vulkaninfo.interimcommandbuffer));
	vulkaninfo.clears[0]={0.0f, 0.0f, 0.01f, 1.0f};
	vulkaninfo.clears[1]={1.0f, 0.0f};
}

void GraphicsHandler::VKSubInitSemaphoresAndFences(){
	vulkaninfo.currentframeinflight=0;
	VkSemaphoreCreateInfo semaphorecreateinfo{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};
	VkFenceCreateInfo fencecreateinfo{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		VK_FENCE_CREATE_SIGNALED_BIT
	};
	for(int x=0;x<MAX_FRAMES_IN_FLIGHT;x++){
		vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphorecreateinfo, nullptr, &vulkaninfo.imageavailablesemaphores[x]);
		vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphorecreateinfo, nullptr, &vulkaninfo.renderfinishedsemaphores[x]);
		vkCreateFence(vulkaninfo.logicaldevice, &fencecreateinfo, nullptr, &vulkaninfo.frameinflightfences[x]);
	}
	for(int x=0;x<vulkaninfo.numswapchainimages;x++){
		vulkaninfo.imageinflightfences[x]=VK_NULL_HANDLE;
	}
}

void GraphicsHandler::VKSubInitPipeline(PipelineInfo*pipelineinfo,
				                        uint32_t numdescriptorsets,
				                        VkDescriptorSetLayout*descriptorsetlayouts,
				                        VkPushConstantRange*pushconstantrange,
				                        VkRenderPass*renderpass,
				                        VkPipelineShaderStageCreateInfo*shadermodules,
				                        VkPipelineVertexInputStateCreateInfo*vertexinputstatecreateinfo){
	VkPipelineLayoutCreateInfo pipelinelayoutcreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		numdescriptorsets,
		&descriptorsetlayouts[0],
		pushconstantrange==nullptr?0u:1u,
		pushconstantrange
	};
	vkCreatePipelineLayout(vulkaninfo.logicaldevice, &pipelinelayoutcreateinfo, nullptr, &pipelineinfo->pipelinelayout);

	VkPipelineInputAssemblyStateCreateInfo inputassemblystatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};
	VkViewport viewporttemp{
		0.0f,
		0.0f,
		(float)vulkaninfo.swapchainextent.width,
		(float)vulkaninfo.swapchainextent.height,
		0.0f,
		1.0f
	};
	VkRect2D scissortemp{
		{0, 0},
		vulkaninfo.swapchainextent
	};
	VkPipelineViewportStateCreateInfo viewportstatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		&viewporttemp,
		1,
		&scissortemp
	};
	VkPipelineRasterizationStateCreateInfo rasterizationstatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};
	VkPipelineMultisampleStateCreateInfo multisamplestatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,
		1.0f,
		nullptr,
		VK_FALSE,
		VK_FALSE
	};
	VkPipelineDepthStencilStateCreateInfo depthstencilstatecreateinfo{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS,
			VK_FALSE,
			VK_FALSE,
			{},
			{},
			0.0f,
			1.0f
	};
	//would like to figure out how this blend state works better
	VkPipelineColorBlendAttachmentState colorblendattachmentstate{
		VK_TRUE,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_SUBTRACT,
		VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo colorblendstatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_LOGIC_OP_AND,
		1,
		&colorblendattachmentstate,
		{0.0f, 0.0f, 0.0f, 0.0f}
	};
	VkGraphicsPipelineCreateInfo pipelinecreateinfo={
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		2,
		shadermodules,
		vertexinputstatecreateinfo,
		&inputassemblystatecreateinfo,
		nullptr,
		&viewportstatecreateinfo,
		&rasterizationstatecreateinfo,
		&multisamplestatecreateinfo,
		&depthstencilstatecreateinfo,
		&colorblendstatecreateinfo,
		nullptr,
		pipelineinfo->pipelinelayout,
		*renderpass,
		0,
		VK_NULL_HANDLE,
		-1
	};
	vkCreateGraphicsPipelines(vulkaninfo.logicaldevice, VK_NULL_HANDLE, 1, &pipelinecreateinfo, nullptr, &pipelineinfo->pipeline);
}

void GraphicsHandler::VKSubInitLoadShaders(const char*vertexshaderfilepath,
		                                   const char*fragmentshaderfilepath,
		                                   VkShaderModule*vertexshadermodule,
		                                   VkShaderModule*fragmentshadermodule,
		                                   VkPipelineShaderStageCreateInfo*vertexshaderstatecreateinfo,
		                                   VkPipelineShaderStageCreateInfo*fragmentshaderstatecreateinfo){
	std::ifstream shaderstream=std::ifstream(vertexshaderfilepath, std::ios::ate|std::ios::binary);
	size_t vertshadersize=shaderstream.tellg(), fragshadersize;
	shaderstream.seekg(0);
	char vertshadersrc[vertshadersize];
	shaderstream.read(&vertshadersrc[0], vertshadersize);
	shaderstream.close();
	shaderstream=std::ifstream(fragmentshaderfilepath, std::ios::ate|std::ios::binary);
	fragshadersize=shaderstream.tellg();
	shaderstream.seekg(0);
	char fragshadersrc[fragshadersize];
	shaderstream.read(&fragshadersrc[0], fragshadersize);
	shaderstream.close();

	VkShaderModuleCreateInfo vertshadermodcreateinfo{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		vertshadersize,
		reinterpret_cast<const uint32_t*>(vertshadersrc)
	}, fragshadermodcreateinfo{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		fragshadersize,
		reinterpret_cast<const uint32_t*>(fragshadersrc)
	};
	vkCreateShaderModule(vulkaninfo.logicaldevice, &vertshadermodcreateinfo, nullptr, vertexshadermodule);
	vkCreateShaderModule(vulkaninfo.logicaldevice, &fragshadermodcreateinfo, nullptr, fragmentshadermodule);

	*vertexshaderstatecreateinfo={
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		VK_SHADER_STAGE_VERTEX_BIT,
		*vertexshadermodule,
		"main",
		nullptr
	};
	*fragmentshaderstatecreateinfo={
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		*fragmentshadermodule,
		"main",
		nullptr
	};
	//consider deleting shader modules after we get it working
}

void GraphicsHandler::VKSubInitDepthBuffer(){
	VkImageCreateInfo imgcreateinfo{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_D32_SFLOAT,
		{vulkaninfo.swapchainextent.width, vulkaninfo.swapchainextent.height, 1},
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,        //apparently this development device's implementation doesn't support tiling linear
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(vulkaninfo.logicaldevice, &imgcreateinfo, nullptr, &vulkaninfo.depthbuffer.image);

	VkMemoryRequirements memrequirements{};
	vkGetImageMemoryRequirements(vulkaninfo.logicaldevice, vulkaninfo.depthbuffer.image, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
	uint32_t memindex=-1u;
	for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
		if(memrequirements.memoryTypeBits&(1<<x)
		   &&((physicaldevicememprops.memoryTypes[x].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)==VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
			memindex=x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, &(vulkaninfo.depthbuffer.memory));
	vkBindImageMemory(vulkaninfo.logicaldevice, vulkaninfo.depthbuffer.image, vulkaninfo.depthbuffer.memory, 0);

	VkImageViewCreateInfo imgviewcreateinfo{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			vulkaninfo.depthbuffer.image,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_D32_SFLOAT,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{
					VK_IMAGE_ASPECT_DEPTH_BIT,
					0,
					1,
					0,
					1
			}
	};
	vkCreateImageView(vulkaninfo.logicaldevice, &imgviewcreateinfo, nullptr, &vulkaninfo.depthbuffer.imageview);

	GraphicsHandler::VKHelperTransitionImageLayout(vulkaninfo.depthbuffer.image,
												   VK_FORMAT_D32_SFLOAT,
												   VK_IMAGE_LAYOUT_UNDEFINED,
												   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void GraphicsHandler::VKHelperCreateAndAllocateBuffer(VkBuffer*buffer,
												VkDeviceSize buffersize,
												VkBufferUsageFlags bufferusage,
												VkDeviceMemory*buffermemory,
												VkMemoryPropertyFlags reqprops){
	VkBufferCreateInfo buffercreateinfo{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		buffersize,
		bufferusage,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};
	vkCreateBuffer(vulkaninfo.logicaldevice, &buffercreateinfo, nullptr, buffer);

	VkMemoryRequirements memrequirements{};
	vkGetBufferMemoryRequirements(vulkaninfo.logicaldevice, *buffer, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
	uint32_t memindex=-1u;
	for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
		if(memrequirements.memoryTypeBits&(1<<x)
		   &&((physicaldevicememprops.memoryTypes[x].propertyFlags&reqprops)==reqprops)){
			memindex=x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, buffermemory);
	vkBindBufferMemory(vulkaninfo.logicaldevice, *buffer, *buffermemory, 0);
}

void GraphicsHandler::VKHelperCpyBufferToBuffer(VkBuffer*src,
												VkBuffer*dst,
												VkDeviceSize size){
	VkBufferCopy region{
			0,
			0,
			size
	};
	VkCommandBufferBeginInfo cmdbuffbegininfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	vkBeginCommandBuffer(vulkaninfo.interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdCopyBuffer(vulkaninfo.interimcommandbuffer, *src, *dst, 1, &region);
	vkEndCommandBuffer(vulkaninfo.interimcommandbuffer);
	VkSubmitInfo subinfo{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&(vulkaninfo.interimcommandbuffer),
			0,
			nullptr
	};
	//could be using a different queue for these transfer operations
	vkQueueSubmit(vulkaninfo.graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkaninfo.graphicsqueue);
}

void GraphicsHandler::VKHelperCpyBufferToImage(VkBuffer*src,
											   VkImage*dst,
											   uint32_t rowpitch,
											   uint32_t horires,
											   uint32_t vertres){
	VkBufferImageCopy imgcopy{
			0,
			rowpitch,
			vertres,
			{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					0,
					1
			}, {
					0,
					0,
					0
			}, {
					horires,
					vertres,
					1
			}
	};
	VkCommandBufferBeginInfo cmdbuffbegininfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	vkBeginCommandBuffer(vulkaninfo.interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdCopyBufferToImage(vulkaninfo.interimcommandbuffer, *src, *dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgcopy);
	vkEndCommandBuffer(vulkaninfo.interimcommandbuffer);
	VkSubmitInfo subinfo{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&(vulkaninfo.interimcommandbuffer),
			0,
			nullptr
	};
	vkQueueSubmit(vulkaninfo.graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkaninfo.graphicsqueue);
}

void GraphicsHandler::VKHelperTransitionImageLayout(VkImage image,
											VkFormat format,
											VkImageLayout oldlayout,
											VkImageLayout newlayout){
	VkCommandBufferBeginInfo cmdbuffbegininfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	VkImageMemoryBarrier imgmembarrier{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			0,
			0,
			oldlayout,
			newlayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{
//					VK_IMAGE_ASPECT_COLOR_BIT,
					format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					1,
					0,
					1
			}
	};
	VkPipelineStageFlags srcmask, dstmask;
	if(oldlayout==VK_IMAGE_LAYOUT_UNDEFINED&&newlayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
		imgmembarrier.srcAccessMask=0;
		imgmembarrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
		srcmask=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstmask=VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(oldlayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL&&newlayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		imgmembarrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
		imgmembarrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
		srcmask=VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstmask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if(oldlayout==VK_IMAGE_LAYOUT_UNDEFINED&&newlayout==VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
		imgmembarrier.srcAccessMask=0;
		imgmembarrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcmask=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstmask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	vkBeginCommandBuffer(vulkaninfo.interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdPipelineBarrier(vulkaninfo.interimcommandbuffer, srcmask, dstmask, 0, 0, nullptr, 0, nullptr, 1, &imgmembarrier);
	vkEndCommandBuffer(vulkaninfo.interimcommandbuffer);
	VkSubmitInfo subinfo{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&(vulkaninfo.interimcommandbuffer),
			0,
			nullptr
	};
	vkQueueSubmit(vulkaninfo.graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkaninfo.graphicsqueue);
}

void GraphicsHandler::VKHelperInitTexture(TextureInfo*texturedst,           //may want to actually break this function up into sub-functions with more diverse usage
					                      uint32_t horires,
					                      uint32_t vertres,
					                      void*data,
					                      uint32_t pixelsize,
					                      VkFormat format){
	VkImageCreateInfo imgcreateinfo{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			format,
			{horires, vertres, 1},
			1,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_LINEAR,     //perhaps add more tiling control later (e.g. if square po2 text
			VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(vulkaninfo.logicaldevice, &imgcreateinfo, nullptr, &(texturedst->image));
	VkSubresourceLayout subresourcelayout;
	VkImageSubresource imgsubresource{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			0
	};
	vkGetImageSubresourceLayout(vulkaninfo.logicaldevice, texturedst->image, &imgsubresource, &subresourcelayout);
	VkDeviceSize size=subresourcelayout.size;

	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&(vulkaninfo.stagingbuffer),
	                                                 size,
	                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                                                 &(vulkaninfo.stagingbuffermemory),
	                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	//okay this was the root of the problem: i had to make a temp void* instead of just using the function
	//arguement data i pass in; ig vkMapMemory only detects change, not initial value
	void*datatemp=nullptr;
	vkMapMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, 0, size, 0, &datatemp);
	char*dstscanptr=reinterpret_cast<char*>(datatemp), *srcscanptr=reinterpret_cast<char*>(data);
	for(uint32_t x=0;x<vertres;x++){
		memcpy(reinterpret_cast<void*>(dstscanptr), reinterpret_cast<void*>(srcscanptr), horires*pixelsize);
		dstscanptr+=subresourcelayout.rowPitch;
		srcscanptr+=horires*pixelsize;
	}
	vkUnmapMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory);

	VkMemoryRequirements memrequirements{};
	vkGetImageMemoryRequirements(vulkaninfo.logicaldevice, texturedst->image, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
	uint32_t memindex=-1u;
	for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
		if(memrequirements.memoryTypeBits&(1<<x)
		   &&((physicaldevicememprops.memoryTypes[x].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)==VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
			memindex=x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, &(texturedst->memory));
	vkBindImageMemory(vulkaninfo.logicaldevice, texturedst->image, texturedst->memory, 0);

	VKHelperTransitionImageLayout(texturedst->image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VKHelperCpyBufferToImage(&(vulkaninfo.stagingbuffer), &texturedst->image, subresourcelayout.rowPitch/pixelsize, horires, vertres);
	vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffer, nullptr);
	VKHelperTransitionImageLayout(texturedst->image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageViewCreateInfo imgviewcreateinfo{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			texturedst->image,
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	vkCreateImageView(vulkaninfo.logicaldevice, &imgviewcreateinfo, nullptr, &(texturedst->imageview));

	if(texturedst->sampler==VK_NULL_HANDLE){
		VkSamplerCreateInfo samplercreateinfo{
				VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				nullptr,
				0,
				VK_FILTER_NEAREST,
				VK_FILTER_NEAREST,
				VK_SAMPLER_MIPMAP_MODE_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				0.0f,
				VK_FALSE,
				1.0f,
				VK_FALSE,
				VK_COMPARE_OP_ALWAYS,
				0.0f,
				1.0f,
				VK_BORDER_COLOR_INT_OPAQUE_BLACK,
				VK_FALSE
		};
		vkCreateSampler(vulkaninfo.logicaldevice, &samplercreateinfo, nullptr, &(texturedst->sampler));
	}
}


VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsHandler::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT*callbackdata,
                                                    void*userdata){
//	std::cout<<"Validation message: "
//	<<"\nSeverity: "<<string_VkDebugUtilsMessageSeverityFlagBitsEXT(severity)
//	<<"\nType    : "<<string_VkDebugUtilsMessageTypeFlagsEXT(type);
//	if(callbackdata->pObjects!=nullptr){
//		std::cout<<"\nObjects : ";
//		std::cout<<(callbackdata->pObjects[0].pObjectName==nullptr?"unnamed":callbackdata->pObjects[0].pObjectName)
//		         <<" ("<<string_VkObjectType(callbackdata->pObjects[0].objectType)<<" @"
//		         <<std::hex<<callbackdata->pObjects[0].objectHandle<<std::dec<<')';
//		for(uint32_t x=1;x<callbackdata->objectCount;x++){
//			std::cout<<"\n          "<<(callbackdata->pObjects[x].pObjectName==nullptr?"unnamed":callbackdata->pObjects[x].pObjectName)
//			<<" ("<<string_VkObjectType(callbackdata->pObjects[x].objectType)<<" @"
//            <<std::hex<<callbackdata->pObjects[x].objectHandle<<std::dec<<')';
//		}
//	}
//	if(callbackdata->pCmdBufLabels!=nullptr){
//		std::cout<<"\nCommand Buffer Labels: ";
//		std::cout<<(callbackdata->pCmdBufLabels[0].pLabelName==nullptr?"unnamed":callbackdata->pCmdBufLabels[0].pLabelName)
//				 <<" (color: ";
//		PRINT_QUAD(callbackdata->pCmdBufLabels[0].color);
//		std::cout<<')';
//		for(uint32_t x=1;x<callbackdata->cmdBufLabelCount;x++){
//			std::cout<<'\n'<<(callbackdata->pCmdBufLabels[x].pLabelName==nullptr?"unnamed":callbackdata->pCmdBufLabels[x].pLabelName)
//			         <<" (color: ";
//			PRINT_QUAD(callbackdata->pCmdBufLabels[x].color);
//			std::cout<<')';
//		}
//	}
//	std::cout<<"\nMessage: "<<callbackdata->pMessage<<'\n';
//
//	std::cout<<callbackdata->pMessage<<std::endl;
	if(severity<VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return VK_FALSE;
	std::cout<<callbackdata->pMessage<<std::endl;
	return VK_FALSE;
}

VkResult GraphicsHandler::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT*createinfo,
                                             const VkAllocationCallbacks*allocator,
                                             VkDebugUtilsMessengerEXT*debugutilsmessenger){
	auto function=(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	return function(instance, createinfo, allocator, debugutilsmessenger);
}

void GraphicsHandler::DestroyDebugUtilsMessengerEXT(VkInstance instance,
											  VkDebugUtilsMessengerEXT debugutilsmessenger,
											  const VkAllocationCallbacks*allocator){
	auto function=(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	function(instance, debugutilsmessenger, allocator);
}