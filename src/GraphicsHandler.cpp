//
// Created by Daniel Paavola on 5/30/21.
//

#include "GraphicsHandler.h"

VulkanInfo GraphicsHandler::vulkaninfo{};

GraphicsHandler::GraphicsHandler(){
	if(vulkaninfo.window==nullptr) VKInit();
	lights=std::vector<Light*>();
	lights.push_back(new Light(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), 32, LIGHT_TYPE_SUN));
	staticshadowcastingmeshes=std::vector<const Mesh*>();
//	staticshadowcastingmeshes.push_back(new Mesh("../resources/objs/smoothhipolysuzanne.obj", glm::vec3(0, 0, 0), &vulkaninfo));
	landscapemeshes=std::vector<Mesh*>();
	landscapemeshes.push_back(new Mesh("../resources/objs/lowpolybeach.obj", glm::vec3(0, 0, 0), &vulkaninfo));
	primarycamera=new Camera(vulkaninfo.window, vulkaninfo.horizontalres, vulkaninfo.verticalres);
	primarygraphicspushconstants={glm::mat4(1), glm::mat4(1)};
	physicshandler=PhysicsHandler(landscapemeshes[0], primarycamera);
	troubleshootingtext=new Text("troubleshooting text", glm::vec2(0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 144.0f, vulkaninfo.horizontalres, vulkaninfo.verticalres);
	lasttime=std::chrono::high_resolution_clock::now();
//	recordCommandBuffers();
}

GraphicsHandler::~GraphicsHandler(){
	vkWaitForFences(vulkaninfo.logicaldevice, MAX_FRAMES_IN_FLIGHT, &vulkaninfo.frameinflightfences[0], VK_TRUE, UINT64_MAX);
	vkQueueWaitIdle(vulkaninfo.graphicsqueue);
	vkQueueWaitIdle(vulkaninfo.presentationqueue);
	vkDeviceWaitIdle(vulkaninfo.logicaldevice);

	delete troubleshootingtext;
	for(auto&m:landscapemeshes) delete m;
	for(auto&m:staticshadowcastingmeshes) delete m;
	delete primarycamera;

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
	VKSubInitDescriptorPool(2u);
	VKSubInitGraphicsPipeline();
	VKSubInitTextGraphicsPipeline();
	VKSubInitFramebuffers();
	VKSubInitCommandPool();
	VKSubInitSemaphoresAndFences();
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
}       //maybe clean this up later? redo how queues and queue indices and queue families are managed???

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
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
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
	VkAttachmentReference attachmentref{
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpassdescription{
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		1,
		&attachmentref,
		nullptr,
		nullptr,
		0,
		nullptr
	};
	VkSubpassDependency subpassdependency{
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0
	};
	VkRenderPassCreateInfo renderpasscreateinfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		1,
		&attachmentdescription,
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
		vulkaninfo.numswapchainimages
	}, {
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		vulkaninfo.numswapchainimages*nummeshes+1
	}};
	VkDescriptorPoolCreateInfo descriptorpoolcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,      //perhaps look into flags
		vulkaninfo.numswapchainimages*(1+nummeshes+1),
		2,
		&descriptorpoolsizes[0]
	};
	vkCreateDescriptorPool(vulkaninfo.logicaldevice, &descriptorpoolcreateinfo, nullptr, &vulkaninfo.descriptorpool);
}

void GraphicsHandler::VKSubInitUniformBuffer(PipelineInfo*pipelineinfo,
											 uint32_t binding,
											 VkDeviceSize buffersize,
											 VkBuffer*buffers,
											 VkDeviceMemory*memories){
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		VkBufferCreateInfo buffcreateinfo{
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				nullptr,
				0,
				buffersize,
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

		VkDescriptorBufferInfo descriptorbuffinfo{
				buffers[x],
				0,
				buffersize
		};
		VkWriteDescriptorSet writedescriptorset{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				pipelineinfo->descriptorset[x],
				binding,
				0,
				1,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				nullptr,
				&descriptorbuffinfo,
				nullptr
		};
		vkUpdateDescriptorSets(vulkaninfo.logicaldevice, 1, &writedescriptorset, 0, nullptr);
	}
}

void GraphicsHandler::VKSubInitGraphicsPipeline(){
//	VkDescriptorSetLayoutBinding descriptorsetlayoutbindings[2]{{
//		0,
//		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		1,
//		VK_SHADER_STAGE_FRAGMENT_BIT,
//		nullptr
//	}, {
//		0,
//		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//		1,
//		VK_SHADER_STAGE_FRAGMENT_BIT,
//		nullptr
//	}};
//	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfos[2]{{
//		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//		nullptr,
//		0,       //look into flags later
//		1,
//		&descriptorsetlayoutbindings[0]
//	}, {
//		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//		nullptr,
//		0,
//		1,
//		&descriptorsetlayoutbindings[1]
//	}};
	VkDescriptorSetLayoutBinding scenedslbinding{
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
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
	std::cout<<"scene descriptor set layout create result: "<<string_VkResult(vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &scenedslcreateinfo, nullptr, &vulkaninfo.primarygraphicspipeline.scenedescriptorsetlayout))<<std::endl;
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
	std::cout<<"mesh descriptor set layout create result: "<<string_VkResult(vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &meshdslcreateinfo, nullptr, &vulkaninfo.primarygraphicspipeline.meshdescriptorsetlayout))<<std::endl;

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
	VKSubInitUniformBuffer(&vulkaninfo.primarygraphicspipeline, 0, sizeof(LightUniformBuffer), vulkaninfo.lightuniformbuffers, vulkaninfo.lightuniformbuffermemories);

	VkPushConstantRange pushconstantrange{
		VK_SHADER_STAGE_VERTEX_BIT,
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
		VkFramebufferCreateInfo framebuffercreateinfo{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			vulkaninfo.primaryrenderpass,
			1,
			&vulkaninfo.swapchainimageviews[x],
			vulkaninfo.swapchainextent.width,
			vulkaninfo.swapchainextent.height,
			1
		};
		vkCreateFramebuffer(vulkaninfo.logicaldevice, &framebuffercreateinfo, nullptr, &vulkaninfo.framebuffers[x]);
	}
}

//do we need to record???
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
	//do we need below record???
	VkClearValue clearcolor{0.0f, 0.0f, 0.01f, 1.0f};
	vulkaninfo.commandbufferinheritanceinfos=new VkCommandBufferInheritanceInfo[vulkaninfo.numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		VkCommandBufferBeginInfo cmdbufferbegininfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				0,
				nullptr
		};
		vulkaninfo.commandbufferinheritanceinfos[x]={
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			vulkaninfo.primaryrenderpass,
			0,
			vulkaninfo.framebuffers[x],
			VK_FALSE,
			0,
			0
		};

		vkBeginCommandBuffer(vulkaninfo.commandbuffers[x], &cmdbufferbegininfo);
		VkRenderPassBeginInfo renderpassbegininfo{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				nullptr,
				vulkaninfo.primaryrenderpass,
				vulkaninfo.framebuffers[x],
				{{0, 0}, vulkaninfo.swapchainextent},
				1,
				&clearcolor
		};
		vkCmdBeginRenderPass(vulkaninfo.commandbuffers[x], &renderpassbegininfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vkCmdEndRenderPass(vulkaninfo.commandbuffers[x]);
		vkEndCommandBuffer(vulkaninfo.commandbuffers[x]);
	}
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

void GraphicsHandler::recordCommandBuffer(uint32_t index){
	//should technically move the clear value out to vulkaninfo
	VkClearValue clearcolor{0.0f, 0.0f, 0.01f, 1.0f};
//		vkResetCommandBuffer(vulkaninfo.commandbuffers[x], 0);
	VkCommandBufferBeginInfo cmdbufferbegininfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			0,
			nullptr
	};

	vkBeginCommandBuffer(vulkaninfo.commandbuffers[index], &cmdbufferbegininfo);
	VkRenderPassBeginInfo renderpassbegininfo{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			vulkaninfo.primaryrenderpass,
			vulkaninfo.framebuffers[index],
			{{0, 0}, vulkaninfo.swapchainextent},
			1,
			&clearcolor
	};
	vkCmdBeginRenderPass(vulkaninfo.commandbuffers[index], &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(vulkaninfo.commandbuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo.primarygraphicspipeline.pipeline);
	vkCmdPushConstants(vulkaninfo.commandbuffers[index],
					   vulkaninfo.primarygraphicspipeline.pipelinelayout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(PrimaryGraphicsPushConstants),
					   (void*)(&primarygraphicspushconstants));
	vkCmdBindDescriptorSets(vulkaninfo.commandbuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo.primarygraphicspipeline.pipelinelayout, 0, 1, &vulkaninfo.primarygraphicspipeline.descriptorset[index], 0, nullptr);
	//consider not even using secondary command buffers, but rather just writing all the rendering code in here, as we have to rebind pipelines otherwise
	//i don't really think this is what secondary command buffers were made for
//		for(auto&m:landscapemeshes) vkCmdExecuteCommands(vulkaninfo.commandbuffers[x], 1, &m->getCommandBuffers()[x]);
	VkDeviceSize offsettemp=0u;
	for(auto&m:landscapemeshes){
		vkCmdBindDescriptorSets(vulkaninfo.commandbuffers[index],
						        VK_PIPELINE_BIND_POINT_GRAPHICS,
						        vulkaninfo.primarygraphicspipeline.pipelinelayout,
						        1,
						        1,
						        &m->getDescriptorSets()[index],
						        0,
						        nullptr);
		vkCmdBindVertexBuffers(vulkaninfo.commandbuffers[index], 0, 1, m->getVertexBuffer(), &offsettemp);
		vkCmdBindIndexBuffer(vulkaninfo.commandbuffers[index], *m->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(vulkaninfo.commandbuffers[index], m->getTris().size()*3, 1, 0 ,0, 0);
	}
	for(auto&m:staticshadowcastingmeshes){
		vkCmdBindDescriptorSets(vulkaninfo.commandbuffers[index],
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        vulkaninfo.primarygraphicspipeline.pipelinelayout,
		                        1,
		                        1,
		                        &m->getDescriptorSets()[index],
		                        0,
		                        nullptr);
		vkCmdBindVertexBuffers(vulkaninfo.commandbuffers[index], 0, 1, m->getVertexBuffer(), &offsettemp);
		vkCmdBindIndexBuffer(vulkaninfo.commandbuffers[index], *m->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(vulkaninfo.commandbuffers[index], m->getTris().size()*3, 1, 0 ,0, 0);
	}
	vkCmdBindPipeline(vulkaninfo.commandbuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo.textgraphicspipeline.pipeline);
	vkCmdPushConstants(vulkaninfo.commandbuffers[index],
					   vulkaninfo.textgraphicspipeline.pipelinelayout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(TextPushConstants),
					   (void*)troubleshootingtext->getPushConstantsPtr());
	vkCmdBindDescriptorSets(vulkaninfo.commandbuffers[index],
						    VK_PIPELINE_BIND_POINT_GRAPHICS,
						    vulkaninfo.textgraphicspipeline.pipelinelayout,
						    0,
						    1,
						    &troubleshootingtext->getDescriptorSetsPtr()[index],
						    0,
						    nullptr);
	vkCmdDraw(vulkaninfo.commandbuffers[index], 6, 1, 0, 0);
	vkCmdEndRenderPass(vulkaninfo.commandbuffers[index]);
	vkEndCommandBuffer(vulkaninfo.commandbuffers[index]);
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
	VkPipelineColorBlendAttachmentState colorblendattachmentstate{
		VK_FALSE,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_OP_ADD,
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
		nullptr,
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

//think about further optimization for semaphores, fences, & frames in flight
void GraphicsHandler::draw(){
	vkWaitForFences(vulkaninfo.logicaldevice, 1, &vulkaninfo.frameinflightfences[vulkaninfo.currentframeinflight], VK_TRUE, UINT64_MAX);

	glfwPollEvents();

	//consider returning bool that signals whether camera has actually changed or not
	//if we had a whole system of change flags for camera, lights, and meshes, we could get a lot of efficiency with data sending and command recording
	primarycamera->takeInputs(vulkaninfo.window);
	physicshandler.updateCameraPos();
	primarygraphicspushconstants={
		primarycamera->getProjectionMatrix()*primarycamera->getViewMatrix(),
		glm::mat4(1)
	};

	//maybe move this to vulkaninfo struct? we don't have to, but it would be tidier, and might prove useful later???
	uint32_t imageindex=-1u;
	vkAcquireNextImageKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, UINT64_MAX, vulkaninfo.imageavailablesemaphores[vulkaninfo.currentframeinflight], VK_NULL_HANDLE, &imageindex);

//	if(glfwGetKey(vulkaninfo.window, GLFW_KEY_G)==GLFW_PRESS){
//		for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
//			VkDescriptorBufferInfo descriptorbuffinfo{
//					vulkaninfo.lightuniformbuffers[0],
//					0,
//					0
//			};
//			VkWriteDescriptorSet writedescriptorset{
//					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//					nullptr,
//					vulkaninfo.primarygraphicspipeline.descriptorset[x],
//					0,
//					0,
//					1,
//					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//					nullptr,
//					&descriptorbuffinfo,
//					nullptr
//			};
//			vkUpdateDescriptorSets(vulkaninfo.logicaldevice, 1, &writedescriptorset, 0, nullptr);
//		}
//	}

	void*datatemp;
	LightUniformBuffer lubtemp{
		primarycamera->getPosition()+glm::vec3(5.0f, 5.0f, 5.0f),
		5.0f,
		glm::vec3(-1.0f, -1.0f, -1.0f),
		LIGHT_TYPE_POINT,
		glm::vec4(sin(glfwGetTime()), sin(glfwGetTime()+2.09f), sin(glfwGetTime()+4.18f), 1.0f)
	};
	vkMapMemory(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffermemories[imageindex], 0, sizeof(LightUniformBuffer), 0, &datatemp);
	memcpy(datatemp, (void*)&lubtemp, sizeof(LightUniformBuffer));
	vkUnmapMemory(vulkaninfo.logicaldevice,  vulkaninfo.lightuniformbuffermemories[imageindex]);

	if(vulkaninfo.imageinflightfences[imageindex]!=VK_NULL_HANDLE){
		vkWaitForFences(vulkaninfo.logicaldevice, 1, &vulkaninfo.imageinflightfences[imageindex], VK_TRUE, UINT64_MAX);
		//can add check here once we figure out change flag system
		recordCommandBuffer(imageindex);
	}
	vulkaninfo.imageinflightfences[imageindex]=vulkaninfo.frameinflightfences[vulkaninfo.currentframeinflight];
	VkPipelineStageFlags pipelinestageflags=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//slight efficiency to be had by holding on to submitinfo and just swapping out parts that change
	//same goes for presentinfo down below
	VkSubmitInfo submitinfo{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		1,
		&vulkaninfo.imageavailablesemaphores[vulkaninfo.currentframeinflight],
		&pipelinestageflags,
		1,
        &vulkaninfo.commandbuffers[imageindex],
        1,
        &vulkaninfo.renderfinishedsemaphores[vulkaninfo.currentframeinflight]
	};
	//so this behavior isn't technically "double"-submitting, but it seems like the frame updates on a different timer than the main loop
	//look in swapchain config
	//note that this double-up isn't represented in the fps calculation, that's a different issue
	vkResetFences(vulkaninfo.logicaldevice, 1, &vulkaninfo.frameinflightfences[vulkaninfo.currentframeinflight]);
	vkQueueSubmit(vulkaninfo.graphicsqueue, 1, &submitinfo, vulkaninfo.frameinflightfences[vulkaninfo.currentframeinflight]);
	VkPresentInfoKHR presentinfo{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&vulkaninfo.renderfinishedsemaphores[vulkaninfo.currentframeinflight],
		1,
		&vulkaninfo.swapchain,
		&imageindex,
		nullptr
	};
	//okay so rate limiter is vkQueuePresentKHR, so if we want better fps, we should investigate optimal options/settings
	vkQueuePresentKHR(vulkaninfo.graphicsqueue, &presentinfo);

	std::cout<<'\r'<<(1.0f/std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-lasttime).count())<<" fps | swapchain image #"<<imageindex<<" | frame in flight #"<<vulkaninfo.currentframeinflight<<" | current tri @"<<physicshandler.getStandingTri();
	vulkaninfo.currentframeinflight=(vulkaninfo.currentframeinflight+1)%MAX_FRAMES_IN_FLIGHT;
	lasttime=std::chrono::high_resolution_clock::now();
}

bool GraphicsHandler::shouldClose(){
	return glfwWindowShouldClose(vulkaninfo.window)||glfwGetKey(vulkaninfo.window, GLFW_KEY_ESCAPE)==GLFW_PRESS;
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
	if(severity!=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) return VK_FALSE;
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