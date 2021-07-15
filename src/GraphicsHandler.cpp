//
// Created by Daniel Paavola on 5/30/21.
//

#include "GraphicsHandler.h"

VulkanInfo GraphicsHandler::vulkaninfo{};

GraphicsHandler::GraphicsHandler(){
	if(vulkaninfo.window==nullptr) VKInit();
	lights=std::vector<Light*>();
	lights.push_back(new Light(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(-1.0f, -1.0f, -1.0f), 32, SUN));
	staticshadowcastingmeshes=std::vector<const Mesh*>();
	landscapemeshes=std::vector<Mesh*>();
	landscapemeshes.push_back(new Mesh("../resources/objs/lowpolybeach.obj", glm::vec3(0, 0, 0), &vulkaninfo));
	primarycamera=new Camera(vulkaninfo.window, vulkaninfo.horizontalres, vulkaninfo.verticalres);
	primarygraphicspushconstants={glm::mat4(1), glm::mat4(1)};
	lasttime=std::chrono::high_resolution_clock::now();
	physicshandler=PhysicsHandler(landscapemeshes[0], primarycamera);
	recordCommandBuffers();
}

//roll through everything when you're bored sometime and update destructor (just cause its good practice)
GraphicsHandler::~GraphicsHandler(){
	for(auto&m:landscapemeshes) delete m;
	for(auto&m:staticshadowcastingmeshes) delete m;
	delete primarycamera;
	vkDeviceWaitIdle(vulkaninfo.logicaldevice);
	for(int x=0;x<MAX_FRAMES_IN_FLIGHT;x++){
		vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.renderfinishedsemaphores[x], nullptr);
		vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.imageavailablesemaphores[x], nullptr);
		vkDestroyFence(vulkaninfo.logicaldevice, vulkaninfo.frameinflightfences[x], nullptr);
	}
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, 1, &(vulkaninfo.interimcommandbuffer));
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, vulkaninfo.numswapchainimages, &vulkaninfo.commandbuffers[0]);
	vkDestroyCommandPool(vulkaninfo.logicaldevice, vulkaninfo.commandpool, nullptr);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) vkDestroyFramebuffer(vulkaninfo.logicaldevice, vulkaninfo.framebuffers[x], nullptr);
	vkDestroyPipeline(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipeline, nullptr);
	vkDestroyRenderPass(vulkaninfo.logicaldevice, vulkaninfo.primaryrenderpass, nullptr);
	vkDestroyPipelineLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipelinelayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.descriptorsetlayout, nullptr);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) vkDestroyImageView(vulkaninfo.logicaldevice, vulkaninfo.swapchainimageviews[x], nullptr);
	vkDestroySwapchainKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, nullptr);
	vkDestroyDevice(vulkaninfo.logicaldevice, nullptr);
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
	delete[] vulkaninfo.queuefamilyindices;
}

void GraphicsHandler::VKInit(){
	VKSubInitWindow();
	VKSubInitInstance();
	VKSubInitDevices();
	VKSubInitQueues();
	VKSubInitSwapchain();
	VKSubInitRenderpass();
	VKSubInitDescriptorPool();
	VKSubInitGraphicsPipeline();
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
}

void GraphicsHandler::VKSubInitInstance(){
	VkApplicationInfo appinfo{
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,
			"Vulkan Sandbox",
			VK_MAKE_VERSION(1, 0, 0),
			"Combustion",
			VK_MAKE_VERSION(1, 0, 0),
			VK_MAKE_API_VERSION(1, 1, 0, 0)
	};

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
			0,
			nullptr,
			instanceextcount,
			&instanceextnames[0]
	};
	vkCreateInstance(&instancecreateinfo, nullptr, &vulkaninfo.instance);
	glfwCreateWindowSurface(vulkaninfo.instance, vulkaninfo.window, nullptr, &vulkaninfo.surface);
}

//maybe clean this up later? redo how queues and queue indices and queue families are managed???
void GraphicsHandler::VKSubInitDevices(){
	uint32_t physicaldevicecount=1, queuefamilycount=-1u, deviceextcount=-1u;
	vkEnumeratePhysicalDevices(vulkaninfo.instance, &physicaldevicecount, &vulkaninfo.physicaldevice);
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, nullptr);
	VkQueueFamilyProperties queuefamilyprops[queuefamilycount];
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, &queuefamilyprops[0]);

	vulkaninfo.queuefamilyindices=new uint32_t[2];
	vulkaninfo.queuefamilyindices[0]=-1u; vulkaninfo.queuefamilyindices[1]=-1u;
	VkBool32 supportssurfacepresentation;
	for(uint32_t x=0;x<queuefamilycount;x++){
		if((queuefamilyprops[x].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&vulkaninfo.queuefamilyindices[0]==-1u) vulkaninfo.queuefamilyindices[0]=x;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkaninfo.physicaldevice, x, vulkaninfo.surface, &supportssurfacepresentation);
		if(supportssurfacepresentation&&vulkaninfo.queuefamilyindices[1]==-1u) vulkaninfo.queuefamilyindices[1]=x;
	}
	float queuepriority=1.0f;
	VkDeviceQueueCreateInfo devicequeuecreateinfos[2]{{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		vulkaninfo.queuefamilyindices[0],
		1,
		&queuepriority
	}, {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		vulkaninfo.queuefamilyindices[1],
		1,
		&queuepriority
	}};
	vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextcount, nullptr);
	VkExtensionProperties deviceextprops[deviceextcount];
	vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextcount, &deviceextprops[0]);
	char*deviceextnames[deviceextcount];
	for(uint32_t x=0;x<deviceextcount;x++) deviceextnames[x]=deviceextprops[x].extensionName;
	VkPhysicalDeviceFeatures physicaldevicefeatures{};
//	physicaldevicefeatures.samplerAnisotropy=VK_TRUE;
	//could include all available, but for now will include none
	VkDeviceCreateInfo devicecreateinfo{
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr,
			0,
			2,
			&devicequeuecreateinfos[0],
			0,
			nullptr,
			deviceextcount,
			&deviceextnames[0],
			&physicaldevicefeatures
	};
	vkCreateDevice(vulkaninfo.physicaldevice, &devicecreateinfo, nullptr, &vulkaninfo.logicaldevice);
}

void GraphicsHandler::VKSubInitQueues(){
	vkGetDeviceQueue(vulkaninfo.logicaldevice, vulkaninfo.queuefamilyindices[0], 0, &vulkaninfo.graphicsqueue);
	vkGetDeviceQueue(vulkaninfo.logicaldevice, vulkaninfo.queuefamilyindices[1], 0, &vulkaninfo.presentationqueue);
}

void GraphicsHandler::VKSubInitSwapchain(){
	VkSurfaceCapabilitiesKHR surfacecaps{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &surfacecaps);
	vulkaninfo.swapchainextent=surfacecaps.currentExtent;
	vulkaninfo.numswapchainimages=surfacecaps.minImageCount;
	bool queuefamilyindicesaresame=vulkaninfo.queuefamilyindices[0]==vulkaninfo.queuefamilyindices[1];
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
		queuefamilyindicesaresame?nullptr:&vulkaninfo.queuefamilyindices[0],
		surfacecaps.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		VK_TRUE,
		VK_NULL_HANDLE
	};
	vkCreateSwapchainKHR(vulkaninfo.logicaldevice, &swapchaincreateinfo, nullptr, &vulkaninfo.swapchain);
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

void GraphicsHandler::VKSubInitDescriptorPool(){
	VkDescriptorPoolSize descriptorpoolsize{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		vulkaninfo.numswapchainimages
	};
	VkDescriptorPoolCreateInfo descriptorpoolcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,      //perhaps look into flags
		vulkaninfo.numswapchainimages,
		1,
		&descriptorpoolsize
	};
	vkCreateDescriptorPool(vulkaninfo.logicaldevice, &descriptorpoolcreateinfo, nullptr, &vulkaninfo.descriptorpool);
}

void GraphicsHandler::VKSubInitGraphicsPipeline(){
	VkDescriptorSetLayoutBinding descriptorsetlayoutbinding{
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,       //look into flags later
		1,
		&descriptorsetlayoutbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &descriptorsetlayoutcreateinfo, nullptr, &vulkaninfo.primarygraphicspipeline.descriptorsetlayout);
	VkDescriptorSetLayout descriptorsetlayoutstemp[vulkaninfo.numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) descriptorsetlayoutstemp[x]=vulkaninfo.primarygraphicspipeline.descriptorsetlayout;
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
	VkDeviceSize buffersize=sizeof(LightUniformBuffer);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		vulkaninfo.lightuniformbuffers[x]=VK_NULL_HANDLE;
		vulkaninfo.lightuniformbuffermemories[x]=VK_NULL_HANDLE;
//		Mesh::createAndAllocateBuffer(&vulkaninfo.lightuniformbuffers[x],
//									  buffersize,
//									  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//									  &vulkaninfo.lightuniformbuffermemories[x],
//									  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
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
		vkCreateBuffer(vulkaninfo.logicaldevice, &buffcreateinfo, nullptr, &vulkaninfo.lightuniformbuffers[x]);
		VkMemoryRequirements memrequirements{};
		vkGetBufferMemoryRequirements(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffers[x], &memrequirements);
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
		vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, &vulkaninfo.lightuniformbuffermemories[x]);
		vkBindBufferMemory(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffers[x], vulkaninfo.lightuniformbuffermemories[x], 0);

		void*datatemp;
		LightUniformBuffer lubtemp={glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};
		vkMapMemory(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffermemories[x], 0, sizeof(LightUniformBuffer), 0, &datatemp);
		memcpy(datatemp, (void*)&lubtemp, sizeof(LightUniformBuffer));
		vkUnmapMemory(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffermemories[x]);

		VkDescriptorBufferInfo descriptorbuffinfo{
			vulkaninfo.lightuniformbuffers[x],
			0,
			sizeof(LightUniformBuffer)
		};
		VkWriteDescriptorSet writedescriptorset{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			vulkaninfo.primarygraphicspipeline.descriptorset[x],
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			nullptr,
			&descriptorbuffinfo,
			nullptr
		};
		vkUpdateDescriptorSets(vulkaninfo.logicaldevice, 1, &writedescriptorset, 0, nullptr);
	}

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
	VkPipelineShaderStageCreateInfo vertshadersci, fragshadersci;
	VKSubInitLoadShaders("../resources/shaders/vert.spv",
					     "../resources/shaders/frag.spv",
					     &vulkaninfo.primarygraphicspipeline.vertexshadermodule,
					     &vulkaninfo.primarygraphicspipeline.fragmentshadermodule,
					     &vertshadersci,
					     &fragshadersci);
	VkPipelineShaderStageCreateInfo shaderstagecreateinfostemp[2]{vertshadersci, fragshadersci};
	VKSubInitPipeline(&vulkaninfo.primarygraphicspipeline.pipelinelayout,
			          &vulkaninfo.primarygraphicspipeline.descriptorsetlayout,
			          &pushconstantrange,
			          &vulkaninfo.primaryrenderpass,
			          &vulkaninfo.primarygraphicspipeline.pipeline,
			          &shaderstagecreateinfostemp[0],
			          &vertinstatecreateinfo);
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
		0,
		vulkaninfo.queuefamilyindices[0]
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
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		VkCommandBufferBeginInfo cmdbufferbegininfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				0,
				nullptr
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
		vkCmdBeginRenderPass(vulkaninfo.commandbuffers[x], &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);
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

void GraphicsHandler::recordCommandBuffers(){
	VkClearValue clearcolor{0.0f, 0.0f, 0.01f, 1.0f};
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		VkCommandBufferBeginInfo cmdbufferbegininfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				0,
				nullptr
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
		vkCmdBeginRenderPass(vulkaninfo.commandbuffers[x], &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(vulkaninfo.commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo.primarygraphicspipeline.pipeline);
		vkCmdPushConstants(vulkaninfo.commandbuffers[x], vulkaninfo.primarygraphicspipeline.pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PrimaryGraphicsPushConstants), (void*)(&primarygraphicspushconstants));
		//look further into below args
		//would like to maybe do this bind in the mesh's cmdbuff
		vkCmdBindDescriptorSets(vulkaninfo.commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo.primarygraphicspipeline.pipelinelayout, 0, 1, &vulkaninfo.primarygraphicspipeline.descriptorset[x], 0, nullptr);
		for(auto&m:landscapemeshes) vkCmdExecuteCommands(vulkaninfo.commandbuffers[x], 1, m->getCommandBuffer());
		//can stick a few optimized checks here for updating dynamic meshes
		for(auto&m:staticshadowcastingmeshes) vkCmdExecuteCommands(vulkaninfo.commandbuffers[x], 1, m->getCommandBuffer());
		vkCmdEndRenderPass(vulkaninfo.commandbuffers[x]);
		vkEndCommandBuffer(vulkaninfo.commandbuffers[x]);
	}
}

void GraphicsHandler::VKSubInitPipeline(VkPipelineLayout*pipelinelayout,
				                        VkDescriptorSetLayout*descriptorsetlayout,
				                        VkPushConstantRange*pushconstantrange,
				                        VkRenderPass*renderpass,
				                        VkPipeline*pipeline,
				                        VkPipelineShaderStageCreateInfo*shadermodules,
				                        VkPipelineVertexInputStateCreateInfo*vertexinputstatecreateinfo){
	VkPipelineLayoutCreateInfo pipelinelayoutcreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		descriptorsetlayout,
		1,
		pushconstantrange
	};
	vkCreatePipelineLayout(vulkaninfo.logicaldevice, &pipelinelayoutcreateinfo, nullptr, pipelinelayout);

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
		*pipelinelayout,
		*renderpass,
		0,
		VK_NULL_HANDLE,
		-1
	};
	vkCreateGraphicsPipelines(vulkaninfo.logicaldevice, VK_NULL_HANDLE, 1, &pipelinecreateinfo, nullptr, pipeline);
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
	//if we had a whole system of change flags for camera, lights, and meshes, we could get a lot of efficency with data sending and command recording
	primarycamera->takeInputs(vulkaninfo.window);
	physicshandler.updateCameraPos();
	primarygraphicspushconstants={
		primarycamera->getProjectionMatrix()*primarycamera->getViewMatrix(),
		glm::mat4(1)
	};

	recordCommandBuffers();

	//maybe move this to vulkaninfo struct? we don't have to, but it would be tidier, and might prove useful later???
	uint32_t imageindex=-1u;
	vkAcquireNextImageKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, UINT64_MAX, vulkaninfo.imageavailablesemaphores[vulkaninfo.currentframeinflight], VK_NULL_HANDLE, &imageindex);
	if(vulkaninfo.imageinflightfences[imageindex]!=VK_NULL_HANDLE) vkWaitForFences(vulkaninfo.logicaldevice, 1, &vulkaninfo.imageinflightfences[imageindex], VK_TRUE, UINT64_MAX);
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
	vkQueuePresentKHR(vulkaninfo.graphicsqueue, &presentinfo);

	std::cout<<'\r'<<(1.0f/std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-lasttime).count())<<" fps | swapchain image #"<<imageindex<<" | frame in flight #"<<vulkaninfo.currentframeinflight;
	vulkaninfo.currentframeinflight=(vulkaninfo.currentframeinflight+1)%MAX_FRAMES_IN_FLIGHT;
	lasttime=std::chrono::high_resolution_clock::now();
}

bool GraphicsHandler::shouldClose(){
	return glfwWindowShouldClose(vulkaninfo.window)||glfwGetKey(vulkaninfo.window, GLFW_KEY_ESCAPE)==GLFW_PRESS;
}

void GraphicsHandler::createGraphicsPipelineSPIRV(const char*vertshaderfilepath, const char*fragshaderfilepath, VkPipelineLayout*pipelinelayout, VkPipelineShaderStageCreateInfo*vertexshaderstagecreateinfo, VkPipelineShaderStageCreateInfo*fragmentshaderstagecreateinfo){
	std::ifstream shaderstream=std::ifstream(vertshaderfilepath, std::ios::ate|std::ios::binary);
	size_t vertshadersize=shaderstream.tellg(), fragshadersize;
	shaderstream.seekg(0);
	char vertshadersrc[vertshadersize];
	shaderstream.read(&vertshadersrc[0], vertshadersize);
	shaderstream.close();
	shaderstream=std::ifstream(fragshaderfilepath, std::ios::ate|std::ios::binary);
	fragshadersize=shaderstream.tellg();
	shaderstream.seekg(0);
	char fragshadersrc[fragshadersize];
	shaderstream.read(&fragshadersrc[0], fragshadersize);
	shaderstream.close();
	VkShaderModuleCreateInfo vertexshadermodulecreateinfo{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		vertshadersize,
		reinterpret_cast<const uint32_t*>(vertshadersrc)
	},
	fragmentshadermodulecreateinfo{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		fragshadersize,
		reinterpret_cast<const uint32_t*>(fragshadersrc)
	};
	VkShaderModule vertexshadermodule, fragmentshadermodule;
	vkCreateShaderModule(vulkaninfo.logicaldevice, &vertexshadermodulecreateinfo, nullptr, &vertexshadermodule);
	vkCreateShaderModule(vulkaninfo.logicaldevice, &fragmentshadermodulecreateinfo, nullptr, &fragmentshadermodule);
	*vertexshaderstagecreateinfo={
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_VERTEX_BIT,
        vertexshadermodule,
        "main",
		nullptr
    };
	*fragmentshaderstagecreateinfo={
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        fragmentshadermodule,
        "main",
        nullptr
	};





	VkDescriptorSetLayoutBinding descriptorsetlayoutbinding{
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr,
	};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&descriptorsetlayoutbinding
	};
	if(vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &descriptorsetlayoutcreateinfo, nullptr, &vulkaninfo.primarygraphicspipeline.descriptorsetlayout)==VK_SUCCESS) std::cout<<"descriptor set layout made\n";





	VkPushConstantRange pushconstantrange{
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PrimaryGraphicsPushConstants)
	};
	VkPipelineLayoutCreateInfo pipelinelayoutcreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&vulkaninfo.primarygraphicspipeline.descriptorsetlayout,
		1,
		&pushconstantrange
	};
	vkCreatePipelineLayout(vulkaninfo.logicaldevice, &pipelinelayoutcreateinfo, nullptr, pipelinelayout);





	VkDescriptorPoolSize descriptorpoolsize{
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		vulkaninfo.numswapchainimages
	};
	VkDescriptorPoolCreateInfo descriptorpoolcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		vulkaninfo.numswapchainimages,
		1,
		&descriptorpoolsize
	};
	if(vkCreateDescriptorPool(vulkaninfo.logicaldevice, &descriptorpoolcreateinfo, nullptr, &(vulkaninfo.descriptorpool))==VK_SUCCESS) std::cout<<"descriptor pool created\n";
	VkDescriptorSetLayout descriptorsetlayoutstemp[vulkaninfo.numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) descriptorsetlayoutstemp[x]=vulkaninfo.primarygraphicspipeline.descriptorsetlayout;
	VkDescriptorSetAllocateInfo descriptorsetallocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		vulkaninfo.descriptorpool,
		vulkaninfo.numswapchainimages,
		&descriptorsetlayoutstemp[0]
	};
	//dont forget to delete in deconstructor, im too lazy to do it now....
	vulkaninfo.primarygraphicspipeline.descriptorset=new VkDescriptorSet[vulkaninfo.numswapchainimages];
	if(vkAllocateDescriptorSets(vulkaninfo.logicaldevice, &descriptorsetallocinfo, &vulkaninfo.primarygraphicspipeline.descriptorset[0])==VK_SUCCESS) std::cout<<"descriptor sets allocated\n";
}