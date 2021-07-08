//
// Created by Daniel Paavola on 5/30/21.
//

#include "GraphicsHandler.h"

VulkanInfo GraphicsHandler::vulkaninfo{};
VkExtent2D GraphicsHandler::swapextent={};

GraphicsHandler::GraphicsHandler(){
//	if(vulkaninfo.window==nullptr) GLInit(true);
	if(vulkaninfo.window==nullptr) VKInit();
	staticshadowcastingmeshes=std::vector<const Mesh*>();
 	staticshadowcastingmeshes.push_back(new Mesh("../resources/objs/smoothhipolysuzanne.obj", glm::vec3(0.0f, 0.0f, 0.0f), &vulkaninfo));
//	staticshadowcastingmeshes.push_back(new Mesh(&vulkaninfo));
	primarycamera=new Camera(vulkaninfo.window, vulkaninfo.horizontalres, vulkaninfo.verticalres);
	primarygraphicspushconstants={glm::mat4(1), glm::mat4(1)};
	lasttime=std::chrono::high_resolution_clock::now();
	recordCommandBuffers();
}

GraphicsHandler::~GraphicsHandler(){
	vkDeviceWaitIdle(vulkaninfo.logicaldevice);
	vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.renderfinishedsemaphore, nullptr);
	vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.imageavailablesemaphore, nullptr);
//	delete primarycamera;
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, 1, &(vulkaninfo.interimcommandbuffer));
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, vulkaninfo.numswapchainimages, &vulkaninfo.commandbuffers[0]);
	vkDestroyCommandPool(vulkaninfo.logicaldevice, vulkaninfo.commandpool, nullptr);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++) vkDestroyFramebuffer(vulkaninfo.logicaldevice, vulkaninfo.framebuffers[x], nullptr);

	vkDestroyPipeline(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipeline, nullptr);
	vkDestroyRenderPass(vulkaninfo.logicaldevice, vulkaninfo.primaryrenderpass, nullptr);
	vkDestroyPipelineLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipelinelayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.descriptorsetlayout, nullptr);
	for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
		vkDestroyImageView(vulkaninfo.logicaldevice, vulkaninfo.swapchainimageviews[x], nullptr);
	}
	//below function seems to take care of vulkaninfo.swapchainimages for me :]
	vkDestroySwapchainKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, nullptr);
	vkDestroyDevice(vulkaninfo.logicaldevice, nullptr);
	vkDestroySurfaceKHR(vulkaninfo.instance, vulkaninfo.surface, nullptr);
	vkDestroyInstance(vulkaninfo.instance, nullptr);
	glfwDestroyWindow(vulkaninfo.window);
	glfwTerminate();
	delete[] vulkaninfo.commandbuffers;
	delete[] vulkaninfo.framebuffers;
	delete[] vulkaninfo.swapchainimageviews;
	delete[] vulkaninfo.swapchainimages;
	delete[] vulkaninfo.queuefamilyindices;
}

//void GraphicsHandler::GLInit(bool printstuff){
//	glfwInit();
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//	const GLFWvidmode*mode=glfwGetVideoMode(glfwGetPrimaryMonitor());
//	horizontalres=mode->width, verticalres=mode->height;
////	vulkaninfo.window=glfwCreateWindow(horizontalres, verticalres, "Vulkan Sandbox", glfwGetPrimaryMonitor(), nullptr);
//	vulkaninfo.window=glfwCreateWindow(800, 600, "Vulkan Sandbox", nullptr, nullptr);
////	glfwMakeContextCurrent(vulkaninfo.window);
//	if(printstuff){
//		std::cout<<(glfwVulkanSupported()==GLFW_TRUE?"glfw supports vulkan!":"vulkan unsupported by glfw")<<std::endl;
//		uint32_t glfwreqinstanceextscount=-1u;
//		const char**glfwreqinstanceextnames=glfwGetRequiredInstanceExtensions(&glfwreqinstanceextscount);
//		std::cout<<"glfw required instance extensions: "<<std::endl;
//		for(uint32_t x=0;x<glfwreqinstanceextscount;x++) std::cout<<'\t'<<glfwreqinstanceextnames[x]<<std::endl;
//	}
//
//	{
//		VkApplicationInfo appinfo={VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Vulkan Sandbox",
//				VK_MAKE_VERSION(1, 0, 0), "Combustion", VK_MAKE_VERSION(1, 0, 0), VK_MAKE_API_VERSION(0, 1, 0, 0)};
//		uint32_t extensioncount=-1u;
//		vkEnumerateInstanceExtensionProperties(nullptr, &extensioncount, nullptr);
//		VkExtensionProperties instanceextensionproperties[extensioncount];
//		vkEnumerateInstanceExtensionProperties(nullptr, &extensioncount, &instanceextensionproperties[0]);
//		char*instanceextensionnames[extensioncount];
//		if(printstuff) std::cout<<"available instance extensions (all will be enabled): "<<std::endl;
//		for(uint32_t x=0;x<extensioncount;x++){
//			instanceextensionnames[x]=instanceextensionproperties[x].extensionName;
//			if(printstuff) std::cout<<'\t'<<instanceextensionnames[x]<<", v"
//				<<instanceextensionproperties[x].specVersion<<std::endl;
//		}
//		uint32_t layercount=-1u;
//		vkEnumerateInstanceLayerProperties(&layercount, nullptr);
//		VkLayerProperties layerprops[layercount];
//		vkEnumerateInstanceLayerProperties(&layercount, &layerprops[0]);
//		if(printstuff){
//			std::cout<<"layers: "<<std::endl;
//			for(uint32_t x=0;x<layercount;x++){
//				std::cout<<'\t'<<layerprops[x].layerName<<std::endl;
//			}
//		}
//		VkInstanceCreateInfo instancecreateinfo={VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, &appinfo, 0,
//	            nullptr, extensioncount, instanceextensionnames};
//		if(vkCreateInstance(&instancecreateinfo, nullptr, &vulkaninfo.instance)==VK_SUCCESS&&printstuff) std::cout<<"vulkan instance successfully made!"<<std::endl;
//		if(glfwCreateWindowSurface(vulkaninfo.instance, vulkaninfo.window, nullptr, &vulkaninfo.surface)==VK_SUCCESS&&printstuff) std::cout<<"vulkan vulkaninfo.surface successfully made!"<<std::endl;
//	}
//
//	//may make these member variables
//	//making a pointer member variable that will get these values, but should go back later and just use pointer
//	uint32_t graphicsqueuefamilyindex=-1u, presentationqueuefamilyindex=-1u;
//	{
//		uint32_t physicaldevicecount=-1u;
//		vkEnumeratePhysicalDevices(vulkaninfo.instance, &physicaldevicecount, nullptr);
//		VkPhysicalDevice physicaldevices[physicaldevicecount];
//		vkEnumeratePhysicalDevices(vulkaninfo.instance, &physicaldevicecount, &physicaldevices[0]);
//		if(printstuff){
//			std::cout<<"physical devices (will default to using first one): "<<std::endl;
//			VkPhysicalDeviceProperties physicaldeviceproperties={};
//			for(int x=0;x<physicaldevicecount;x++){
//				vkGetPhysicalDeviceProperties(physicaldevices[x], &physicaldeviceproperties);
//				std::cout<<'\t'<<physicaldeviceproperties.deviceName<<std::endl;
//			}
//		}
//		vulkaninfo.physicaldevice=physicaldevices[0];
//		uint32_t queuefamilycount=-1u;
//		vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, nullptr);
//		VkQueueFamilyProperties queuefamilyprops[queuefamilycount];
//		vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, &queuefamilyprops[0]);
//		VkBool32 supportspresentation;
//		if(printstuff) std::cout<<"queue families from physical device: "<<std::endl;
//		for(uint32_t x=0;x<queuefamilycount;x++){
//			if((queuefamilyprops[x].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&graphicsqueuefamilyindex==-1u) graphicsqueuefamilyindex=x;
//			vkGetPhysicalDeviceSurfaceSupportKHR(vulkaninfo.physicaldevice, x, vulkaninfo.surface, &supportspresentation);
//			if(supportspresentation&&presentationqueuefamilyindex==-1u) presentationqueuefamilyindex=x;
//			if(printstuff){
//				std::cout<<'\t'<<queuefamilyprops[x].queueCount<<" queues with flags [";
//				if(queuefamilyprops[x].queueFlags&VK_QUEUE_GRAPHICS_BIT) std::cout<<"graphics ";
//				if(queuefamilyprops[x].queueFlags&VK_QUEUE_COMPUTE_BIT) std::cout<<"compute ";
//				if(queuefamilyprops[x].queueFlags&VK_QUEUE_TRANSFER_BIT) std::cout<<"transfer ";
//				if(queuefamilyprops[x].queueFlags&VK_QUEUE_SPARSE_BINDING_BIT) std::cout<<"sparse binding ";
//				if(supportspresentation) std::cout<<"] and supports presentation";
//				std::cout<<std::endl;
//			}
//		}
//		if(printstuff){
//			std::cout<<"selected graphics queue: "<<graphicsqueuefamilyindex<<std::endl;
//			std::cout<<"selected presentation queue: "<<presentationqueuefamilyindex<<std::endl;
//		}
//
//		float queuepriority=1.0f;
//		VkDeviceQueueCreateInfo devicequeuecreateinfos[2]={{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0,
//                graphicsqueuefamilyindex, 1, &queuepriority}, {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr,
//	            0, presentationqueuefamilyindex, 1, &queuepriority}};
//		VkPhysicalDeviceFeatures physicaldevicefeatures={};
//		uint32_t deviceextensioncount=-1u;
//		vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextensioncount, nullptr);
//		VkExtensionProperties extprops[deviceextensioncount];
//		vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextensioncount, &extprops[0]);
//		char*extnames[deviceextensioncount];
//		if(printstuff) std::cout<<"available physical device extensions (all included by default):"<<std::endl;
//		for(uint32_t x=0;x<deviceextensioncount;x++){
//			extnames[x]=extprops[x].extensionName;
//			if(printstuff) std::cout<<'\t'<<extnames[x]<<", v"<<extprops->specVersion<<std::endl;
//		}
//		VkDeviceCreateInfo devicecreateinfo={VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0, 2,
//	            &devicequeuecreateinfos[0], 0, nullptr, deviceextensioncount, &extnames[0], &physicaldevicefeatures};
//		if(vkCreateDevice(vulkaninfo.physicaldevice, &devicecreateinfo, nullptr, &vulkaninfo.logicaldevice)==VK_SUCCESS&&printstuff)
//			std::cout<<"device successfully created!"<<std::endl;
//	}
//
//	{
//		vkGetDeviceQueue(vulkaninfo.logicaldevice, graphicsqueuefamilyindex, 0, &vulkaninfo.graphicsqueue);
//		vkGetDeviceQueue(vulkaninfo.logicaldevice, presentationqueuefamilyindex, 0, &vulkaninfo.presentationqueue);
//	}
//
//	{
//		VkSurfaceCapabilitiesKHR caps={};
//		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &caps);
//		if(printstuff){
//			std::cout<<"physical device vulkaninfo.surface capabilities:"<<std::endl;
//			std::cout<<"\timage count min and max: "<<caps.minImageCount<<"->"<<caps.maxImageCount<<std::endl;
//			std::cout<<"\tmin, max, and current extents: ("<<caps.minImageExtent.width<<", "<<caps.minImageExtent.height
//				<<")->("<<caps.currentExtent.width<<", "<<caps.currentExtent.height<<")->("<<caps.maxImageExtent.width
//				<<", "<<caps.maxImageExtent.height<<')'<<std::endl;
//			std::cout<<"\tmax image array layers: "<<caps.maxImageArrayLayers<<std::endl;
//		}
//		uint32_t formatcount;
//		vkGetPhysicalDeviceSurfaceFormatsKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &formatcount, nullptr);
//		VkSurfaceFormatKHR formats[formatcount];
//		vkGetPhysicalDeviceSurfaceFormatsKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &formatcount, &formats[0]);
//		if(printstuff){
//			std::cout<<"physical device vulkaninfo.surface formats:"<<std::endl;
//			for(uint32_t x=0;x<formatcount;x++){
//				std::cout<<'\t'<<formats[x].format<<", colorspace: "<<formats[x].colorSpace<<std::endl;
//			}
//		}
//		uint32_t presentationmodecount;
//		vkGetPhysicalDeviceSurfacePresentModesKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &presentationmodecount, nullptr);
//		VkPresentModeKHR presentationmodes[presentationmodecount];
//		vkGetPhysicalDeviceSurfacePresentModesKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &presentationmodecount, &presentationmodes[0]);
//		if(printstuff){
//			std::cout<<"physical device vulkaninfo.surface presentation modes:"<<std::endl;
//			for(uint32_t x=0;x<presentationmodecount;x++){
//				std::cout<<'\t'<<presentationmodes[x]<<std::endl;
//			}
//		}
//
//		vulkaninfo.queuefamilyindices=new uint32_t[2];
//		vulkaninfo.queuefamilyindices[0]=graphicsqueuefamilyindex, vulkaninfo.queuefamilyindices[1]=presentationqueuefamilyindex;
////		surfaceformat={VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
//		surfacepresentationmode=VK_PRESENT_MODE_FIFO_KHR;
//		swapextent=caps.currentExtent;
//		//coult likely inline some of these arguements (e.g. format)
//		VkSwapchainCreateInfoKHR swapchaincreateinfo={VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0,
//				vulkaninfo.surface, caps.minImageCount+1, surfaceformat.format, surfaceformat.colorSpace, swapextent,
//				1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SHARING_MODE_MAX_ENUM, 0, nullptr,
//				caps.currentTransform, VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, surfacepresentationmode, VK_TRUE, VK_NULL_HANDLE};
//		if(graphicsqueuefamilyindex==presentationqueuefamilyindex){
//			swapchaincreateinfo.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
//		}
//		else{
//			swapchaincreateinfo.imageSharingMode=VK_SHARING_MODE_CONCURRENT;
//			swapchaincreateinfo.queueFamilyIndexCount=2;
//			swapchaincreateinfo.pQueueFamilyIndices=&vulkaninfo.queuefamilyindices[0];
//		}
//		if(vkCreateSwapchainKHR(vulkaninfo.logicaldevice, &swapchaincreateinfo, nullptr, &vulkaninfo.swapchain)==VK_SUCCESS&&printstuff) std::cout<<"vulkaninfo.swapchain successfully made!!"<<std::endl;
//	}
//
//	{
//		uint32_t swapchainimagecount=-1u;
//		vkGetSwapchainImagesKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, &swapchainimagecount, nullptr);
//		vulkaninfo.numswapchainimages=swapchainimagecount;
//		vulkaninfo.swapchainimages=new VkImage[vulkaninfo.numswapchainimages];
//		vkGetSwapchainImagesKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, &vulkaninfo.numswapchainimages, &vulkaninfo.swapchainimages[0]);
//		vulkaninfo.swapchainimageviews=new VkImageView[vulkaninfo.numswapchainimages];
//		for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
//			VkImageViewCreateInfo imageviewcreateinfo={VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0, vulkaninfo.swapchainimages[x],
//					VK_IMAGE_VIEW_TYPE_2D, surfaceformat.format, {VK_COMPONENT_SWIZZLE_IDENTITY,
//					VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
//	                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
//			if(vkCreateImageView(vulkaninfo.logicaldevice, &imageviewcreateinfo, nullptr, &vulkaninfo.swapchainimageviews[x])==VK_SUCCESS&&printstuff) std::cout<<"image view successfully created"<<std::endl;
//		}
//	}
//
//	createGraphicsPipelineSPIRV("../resources/shaders/vert.spv", "../resources/shaders/frag.spv");
//
//	{
//		//some of my variable naming here is a bit sketch
//		VkAttachmentDescription colorattachment={0, surfaceformat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
//	            VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
//	            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
//		VkAttachmentReference colorattachmentref={0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
//		VkSubpassDescription primarysubpass={0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 1, &colorattachmentref,
//	            nullptr, nullptr, 0, nullptr};
//
//		//would like to understand below line better...
//		VkSubpassDependency subpassdependency={VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//		                                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0};
//
//		VkRenderPassCreateInfo renderpassci={VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &colorattachment,
//	            1, &primarysubpass, 1, &subpassdependency};
//		if(vkCreateRenderPass(vulkaninfo.logicaldevice, &renderpassci, nullptr, &vulkaninfo.primaryrenderpass)==VK_SUCCESS&&printstuff) std::cout<<"renderpass successfully created!!"<<std::endl;
//	}
//
//	{
//		VkPipelineVertexInputStateCreateInfo pvertexinputsci={VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
//		                                                      nullptr, 0, 0, nullptr, 0, nullptr};
//		VkPipelineInputAssemblyStateCreateInfo  pinputassemblysci={VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
//		                                                           nullptr, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE};
//		viewport={0.0f, 0.0f, (float)swapextent.width, (float)swapextent.height, 0.0f, 1.0f};
//		scissor={{0u, 0u}, swapextent};
//		//could try and inline this viewport and scissors stuff later if stuff turns out
//		VkPipelineViewportStateCreateInfo pviewportsci={VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
//		                                                nullptr, 0, 1, &viewport, 1, &scissor};
//		VkPipelineRasterizationStateCreateInfo prasterizationsci={VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
//		                                                          nullptr, 0, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE,
//		                                                          VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f};
//		VkPipelineMultisampleStateCreateInfo pmultisamplesci={VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
//		                                                      nullptr, 0, VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE};
//		//consider enabling in the future
//		VkPipelineColorBlendAttachmentState pcolorblendattachments={VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
//		                                                            VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT|
//		                                                                                                                                         VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT};
//		VkPipelineColorBlendStateCreateInfo pcolorblendsci={VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
//		                                                    nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &pcolorblendattachments, {0.0f, 0.0f, 0.0f, 0.0f}};
//		VkGraphicsPipelineCreateInfo graphicspipelineci={VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0, 2,
//	            &shaderstagecreateinfos[0], &pvertexinputsci, &pinputassemblysci, nullptr, &pviewportsci, &prasterizationsci,
//	            &pmultisamplesci, nullptr, &pcolorblendsci, nullptr, pipelinelayout, vulkaninfo.primaryrenderpass, 0, VK_NULL_HANDLE, -1};
//		if(vkCreateGraphicsPipelines(vulkaninfo.logicaldevice, VK_NULL_HANDLE, 1, &graphicspipelineci, nullptr, &primarygraphicspipeline)==VK_SUCCESS&&printstuff) std::cout<<"graphics pipeline created!!! :)"<<std::endl;
//	}
//
//	{
//		vulkaninfo.framebuffers=new VkFramebuffer[vulkaninfo.numswapchainimages];
//		for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
//			VkFramebufferCreateInfo fbci={VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, vulkaninfo.primaryrenderpass,
//		            1, &vulkaninfo.swapchainimageviews[x], swapextent.width, swapextent.height, 1};
//			if(vkCreateFramebuffer(vulkaninfo.logicaldevice, &fbci, nullptr, &vulkaninfo.framebuffers[x])==VK_SUCCESS&&printstuff) std::cout<<"framebuffer "<<x<<" successfully created"<<std::endl;
//		}
//	}
//
//	{
//		VkCommandPoolCreateInfo commandpoolci={VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, 0, vulkaninfo.queuefamilyindices[0]};
//		if(vkCreateCommandPool(vulkaninfo.logicaldevice, &commandpoolci, nullptr, &vulkaninfo.commandpool)==VK_SUCCESS&&printstuff) std::cout<<"command pool successfully made!"<<std::endl;
//		VkCommandBufferAllocateInfo commandbufferai={VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
//	            vulkaninfo.commandpool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, vulkaninfo.numswapchainimages};
//		vulkaninfo.commandbuffers=new VkCommandBuffer[vulkaninfo.numswapchainimages];
//		if(vkAllocateCommandBuffers(vulkaninfo.logicaldevice, &commandbufferai, &vulkaninfo.commandbuffers[0])==VK_SUCCESS&&printstuff) std::cout<<"command buffers successfully allocated"<<std::endl;
//		for(uint32_t x=0;x<vulkaninfo.numswapchainimages;x++){
//			VkCommandBufferBeginInfo commandbufferbi={VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
//			if(vkBeginCommandBuffer(vulkaninfo.commandbuffers[x], &commandbufferbi)==VK_SUCCESS&&printstuff) std::cout<<"command buffer "<<x<<" recording started"<<std::endl;
//			VkClearValue clearcolor={0.0f, 0.0f, 0.01f, 1.0f};
//			//could i use the scissor for the VkRect2D arguement below?
//			VkRenderPassBeginInfo renderpassbi={VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, vulkaninfo.primaryrenderpass,
//		            vulkaninfo.framebuffers[x], {{0, 0}, swapextent}, 1, &clearcolor};
//			//what is the subpass arguement below?
//			vkCmdBeginRenderPass(vulkaninfo.commandbuffers[x], &renderpassbi, VK_SUBPASS_CONTENTS_INLINE);
//			vkCmdBindPipeline(vulkaninfo.commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, primarygraphicspipeline);
//			vkCmdDraw(vulkaninfo.commandbuffers[x], 3, 1, 0, 0);
//			vkCmdEndRenderPass(vulkaninfo.commandbuffers[x]);
//			if(vkEndCommandBuffer(vulkaninfo.commandbuffers[x])==VK_SUCCESS&&printstuff) std::cout<<"command buffer "<<x<<" recording ended (successfully???)"<<std::endl;
//		}
//	}
//
//	{
//		VkSemaphoreCreateInfo semaphoreci={VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
//		if(vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphoreci, nullptr, &vulkaninfo.imageavailablesemaphore)==VK_SUCCESS&&printstuff) std::cout<<"image available semaphore created"<<std::endl;
//		if(vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphoreci, nullptr, &vulkaninfo.renderfinishedsemaphore)==VK_SUCCESS&&printstuff) std::cout<<"render finished semaphore created"<<std::endl;
//	}
//}

void GraphicsHandler::VKInit(){
	VKSubInitWindow();
	VKSubInitInstance();
	VKSubInitDevices();
	VKSubInitQueues();
	VKSubInitSwapchain();
	VKSubInitRenderpass();
//	createGraphicsPipelineSPIRV("../resources/shaders/vert.spv", "../resources/shaders/frag.spv");
	VKSubInitGraphicsPipeline("../resources/shaders/vert.spv", "../resources/shaders/frag.spv");
	VKSubInitFramebuffers();
	VKSubInitCommandPool();
	VKSubInitSemaphores();
}

void GraphicsHandler::VKSubInitWindow(){
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	const GLFWvidmode*mode=glfwGetVideoMode(glfwGetPrimaryMonitor());
	vulkaninfo.width=mode->width; vulkaninfo.height=mode->height;
//	window=glfwCreateWindow(width, height, "Vulkan Sandbox", nullptr, nullptr);
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

void GraphicsHandler::VKSubInitDevices(){
	uint32_t physicaldevicecount=1, queuefamilycount=-1u, deviceextcount=-1u;
	vkEnumeratePhysicalDevices(vulkaninfo.instance, &physicaldevicecount, &vulkaninfo.physicaldevice);
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, nullptr);
	VkQueueFamilyProperties queuefamilyprops[queuefamilycount];
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, &queuefamilyprops[0]);
	//maybe i should store these as separate variables, not an array??
	vulkaninfo.queuefamilyindices=new uint32_t[2];
	vulkaninfo.queuefamilyindices[0]=-1u; vulkaninfo.queuefamilyindices[1]=-1u;
	VkBool32 supportssurfacepresentation;
	for(uint32_t x=0;x<queuefamilycount;x++){
		if((queuefamilyprops[x].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&vulkaninfo.queuefamilyindices[0]==-1u) vulkaninfo.queuefamilyindices[0]=x;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkaninfo.physicaldevice, x, vulkaninfo.surface, &supportssurfacepresentation);
		if(supportssurfacepresentation&&vulkaninfo.queuefamilyindices[1]==-1u) vulkaninfo.queuefamilyindices[1]=x;
	}
	float queuepriority=1.0f;
	//below struct seemingly has option to do more than one queue???
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
	physicaldevicefeatures.samplerAnisotropy=VK_TRUE;
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
	swapextent=surfacecaps.currentExtent;
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
		swapextent,
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
		VULKAN_NUM_SAMPLES,
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

void GraphicsHandler::VKSubInitGraphicsPipeline(const char*vertexshaderfilepath, const char*fragmentshaderfilepath){
	VkPipelineShaderStageCreateInfo*shaderstagecreateinfos=new VkPipelineShaderStageCreateInfo[2];
	createGraphicsPipelineSPIRV("../resources/shaders/vert.spv", "../resources/shaders/frag.spv", &(vulkaninfo.primarygraphicspipeline.pipelinelayout), &shaderstagecreateinfos[0], &shaderstagecreateinfos[1]);
	VkVertexInputBindingDescription vertexinputbindingdescription{
		0,
		sizeof(Vertex),
		VK_VERTEX_INPUT_RATE_VERTEX
	};
	VkVertexInputAttributeDescription vertexinputattributedescriptions[3]{{
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
	VkPipelineVertexInputStateCreateInfo vertexinputstatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		&vertexinputbindingdescription,
		3,
		&vertexinputattributedescriptions[0]
	};
	VkPipelineInputAssemblyStateCreateInfo inputassemblystatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};
	VkViewport viewport{
		0.0f,
		0.0f,
		(float)swapextent.width,
		(float)swapextent.height,
		0.0f,
		1.0f
	};
	VkRect2D scissor{
		{0, 0},
		swapextent
	};
	VkPipelineViewportStateCreateInfo viewportstatecreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		&viewport,
		1,
		&scissor
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
		VULKAN_NUM_SAMPLES,
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
		//had two G bits here, really fucked shit up...
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
	VkGraphicsPipelineCreateInfo graphicspipelinecreateinfo{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		2,
		&shaderstagecreateinfos[0],
		&vertexinputstatecreateinfo,
		&inputassemblystatecreateinfo,
		nullptr,
		&viewportstatecreateinfo,
		&rasterizationstatecreateinfo,
		&multisamplestatecreateinfo,
		nullptr,
		&colorblendstatecreateinfo,
		nullptr,
		vulkaninfo.primarygraphicspipeline.pipelinelayout,
		vulkaninfo.primaryrenderpass,
		0,
		VK_NULL_HANDLE,
		-1
	};
	vkCreateGraphicsPipelines(vulkaninfo.logicaldevice, VK_NULL_HANDLE, 1, &graphicspipelinecreateinfo, nullptr, &(vulkaninfo.primarygraphicspipeline.pipeline));
	delete[] shaderstagecreateinfos;
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
			swapextent.width,
			swapextent.height,
			1
		};
		vkCreateFramebuffer(vulkaninfo.logicaldevice, &framebuffercreateinfo, nullptr, &vulkaninfo.framebuffers[x]);
	}
}

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
				{{0, 0}, swapextent},
				1,
				&clearcolor
		};
		vkCmdBeginRenderPass(vulkaninfo.commandbuffers[x], &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(vulkaninfo.commandbuffers[x]);
		vkEndCommandBuffer(vulkaninfo.commandbuffers[x]);
	}
}

void GraphicsHandler::VKSubInitSemaphores(){
	VkSemaphoreCreateInfo semaphorecreateinfo{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};
	vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphorecreateinfo, nullptr, &vulkaninfo.imageavailablesemaphore);
	vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphorecreateinfo, nullptr, &vulkaninfo.renderfinishedsemaphore);
}

//void GraphicsHandler::VKSubInitBufferStuff(VkDeviceSize buffersize){
//	Mesh::createAndAllocateBuffer(&(vulkaninfo.stagingbuffer), buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &(vulkaninfo.stagingbuffermemory), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//}

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
				{{0, 0}, swapextent},
				1,
				&clearcolor
		};
		vkCmdBeginRenderPass(vulkaninfo.commandbuffers[x], &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(vulkaninfo.commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo.primarygraphicspipeline.pipeline);
		vkCmdPushConstants(vulkaninfo.commandbuffers[x], vulkaninfo.primarygraphicspipeline.pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PrimaryGraphicsPushConstants), (void*)(&primarygraphicspushconstants));
		//can stick a few optimized checks here for updating dynamic meshes
		for(auto&m:staticshadowcastingmeshes) vkCmdExecuteCommands(vulkaninfo.commandbuffers[x], 1, m->getCommandBuffer());
		vkCmdEndRenderPass(vulkaninfo.commandbuffers[x]);
		vkEndCommandBuffer(vulkaninfo.commandbuffers[x]);
	}
}

void GraphicsHandler::draw(){
	glfwPollEvents();

	primarycamera->takeInputs(vulkaninfo.window);
	primarygraphicspushconstants={
		primarycamera->getProjectionMatrix()*primarycamera->getViewMatrix(),
		glm::mat4(1)
	};

	recordCommandBuffers();

	uint32_t imageindex=-1u;
	vkAcquireNextImageKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, UINT64_MAX, vulkaninfo.imageavailablesemaphore, VK_NULL_HANDLE, &imageindex);
	VkPipelineStageFlags pipelinestageflags=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitinfo{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		1,
		&vulkaninfo.imageavailablesemaphore,
		&pipelinestageflags,
		1,
        &vulkaninfo.commandbuffers[imageindex],
        1,
        &vulkaninfo.renderfinishedsemaphore
	};
	//concerning that this is double submitting queues...
	//so this behavior isn't technically "double"-submitting, but it seems like the frame updates on a different timer than the main loop
	vkQueueSubmit(vulkaninfo.graphicsqueue, 1, &submitinfo, VK_NULL_HANDLE);
	VkPresentInfoKHR presentinfo{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&vulkaninfo.renderfinishedsemaphore,
		1,
		&vulkaninfo.swapchain,
		&imageindex,
		nullptr
	};
	vkQueuePresentKHR(vulkaninfo.graphicsqueue, &presentinfo);
	std::cout<<(1.0f/std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-lasttime).count())<<" fps"<<std::endl;
	lasttime=std::chrono::high_resolution_clock::now();
}

void GraphicsHandler::drawShittyTextureMonitor(){
}

void GraphicsHandler::sendLightUniforms(){
}

bool GraphicsHandler::shouldClose(){
	return glfwWindowShouldClose(vulkaninfo.window)||glfwGetKey(vulkaninfo.window, GLFW_KEY_ESCAPE)==GLFW_PRESS;
}

GLuint GraphicsHandler::linkShaders(char shadertypes, ...){
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
//	VkDescriptorSetLayoutBinding descriptorsetlayoutbinding{
//		0,
//		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		1,
//		VK_SHADER_STAGE_VERTEX_BIT,
//		nullptr
//	};
//	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfo{
//		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//		nullptr,
//		0,
//		1,
//		&descriptorsetlayoutbinding
//	};
//	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &descriptorsetlayoutcreateinfo, nullptr, &(vulkaninfo.primarygraphicspipeline.descriptorsetlayout));
//	VkDeviceSize uniformbuffersize=sizeof(PrimaryGraphicsUniform);
//	Mesh::createAndAllocateBuffer(&(vulkaninfo.primarygraphicspipeline.uniformbuffer), uniformbuffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &(vulkaninfo.primarygraphicspipeline.uniformbuffermemory), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkPushConstantRange pushconstantrange{
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PrimaryGraphicsPushConstants)
	};
	VkPipelineLayoutCreateInfo pipelinelayoutcreateinfo{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		1,
		&pushconstantrange
	};
	vkCreatePipelineLayout(vulkaninfo.logicaldevice, &pipelinelayoutcreateinfo, nullptr, pipelinelayout);
}