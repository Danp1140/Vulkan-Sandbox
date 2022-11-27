//
// Created by Daniel Paavola on 5/30/21.
//

#include "GraphicsHandler.h"

VulkanInfo GraphicsHandler::vulkaninfo {};
ChangeFlag* GraphicsHandler::changeflags = nullptr;
std::stringstream GraphicsHandler::troubleshootingsstrm = std::stringstream();
std::map<int, KeyInfo>GraphicsHandler::keyvalues = std::map<int, KeyInfo>();
uint32_t GraphicsHandler::swapchainimageindex = -1u;
VkSampler GraphicsHandler::genericsampler = VK_NULL_HANDLE,
		GraphicsHandler::linearminsampler = VK_NULL_HANDLE,
		GraphicsHandler::linearminmagsampler = VK_NULL_HANDLE,
		GraphicsHandler::clamptobordersampler = VK_NULL_HANDLE,
		GraphicsHandler::depthsampler = VK_NULL_HANDLE;

GraphicsHandler::GraphicsHandler () {}

GraphicsHandler::~GraphicsHandler () {
//	vkWaitForFences(vulkaninfo.logicaldevice, MAX_FRAMES_IN_FLIGHT, &vulkaninfo.frameinflightfences[0], VK_TRUE, UINT64_MAX);
	vkQueueWaitIdle(vulkaninfo.graphicsqueue);
	vkQueueWaitIdle(vulkaninfo.presentationqueue);
	vkDeviceWaitIdle(vulkaninfo.logicaldevice);
	for (int x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.renderfinishedsemaphores[x], nullptr);
		vkDestroySemaphore(vulkaninfo.logicaldevice, vulkaninfo.imageavailablesemaphores[x], nullptr);
//		vkDestroyFence(vulkaninfo.logicaldevice, vulkaninfo.frameinflightfences[x], nullptr);
	}
	vkDestroyDescriptorPool(vulkaninfo.logicaldevice, vulkaninfo.descriptorpool, nullptr);
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, 1, &(vulkaninfo.interimcommandbuffer));
	vkFreeCommandBuffers(vulkaninfo.logicaldevice, vulkaninfo.commandpool, vulkaninfo.numswapchainimages,
						 &vulkaninfo.commandbuffers[0]);
	vkDestroyCommandPool(vulkaninfo.logicaldevice, vulkaninfo.commandpool, nullptr);
	for (uint32_t x = 0; x < vulkaninfo.numswapchainimages; x++) {
		vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffers[x], nullptr);
		vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.lightuniformbuffermemories[x], nullptr);
		vkDestroyFramebuffer(vulkaninfo.logicaldevice, vulkaninfo.primaryframebuffers[x], nullptr);
	}
	vkDestroyPipeline(vulkaninfo.logicaldevice, vulkaninfo.textgraphicspipeline.pipeline, nullptr);
	vkDestroyPipeline(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipeline, nullptr);
	vkDestroyRenderPass(vulkaninfo.logicaldevice, vulkaninfo.primaryrenderpass, nullptr);
	vkDestroyPipelineLayout(vulkaninfo.logicaldevice, vulkaninfo.textgraphicspipeline.pipelinelayout, nullptr);
	vkDestroyPipelineLayout(vulkaninfo.logicaldevice, vulkaninfo.primarygraphicspipeline.pipelinelayout, nullptr);
	for (uint32_t x = 0; x < vulkaninfo.numswapchainimages; x++)
		vkDestroyImageView(vulkaninfo.logicaldevice, vulkaninfo.swapchainimageviews[x], nullptr);
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
	delete[] vulkaninfo.primaryframebuffers;
	delete[] vulkaninfo.swapchainimageviews;
	delete[] vulkaninfo.swapchainimages;
}

void GraphicsHandler::VKInit (uint32_t nummeshes) {
	VKSubInitWindow();
	VKSubInitInstance();
	VKSubInitDebug();
	VKSubInitDevices();
	VKSubInitQueues();
	VKSubInitSwapchain();
	VKSubInitRenderpasses();
	VKSubInitCommandPool();
	VKSubInitDepthBuffer();
	VKSubInitFramebuffers();
	VKSubInitSemaphoresAndFences();
	VKSubInitDescriptorLayoutsAndPool(nummeshes);
	VKSubInitSamplers();
}

void GraphicsHandler::VKInitPipelines () {
	//default
	{
		VkDescriptorSetLayoutBinding objecttexdslbindings[5] {
				{
						0,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						1,
						VK_SHADER_STAGE_VERTEX_BIT,
						nullptr
				},
				{
						1,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						1,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				},
				{
						2,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						1,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				},
				{
						3,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						1,
						VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				},
				{
						4,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						MAX_LIGHTS,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				}};
		VkDescriptorSetLayoutCreateInfo descsetlayoutcreateinfos[3] {
				scenedslcreateinfo,
				{
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						5,
						&objecttexdslbindings[0]
				}};
		VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
		VkVertexInputAttributeDescription vertinattribdesc[3] {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,
																										   position)},
															   {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,
																										   normal)},
															   {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex,
																										uv)}};
		const char* shaderfilepathstemp[4] = {WORKING_DIRECTORY "/resources/shaders/SPIRV/defaultvert.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/defaulttesc.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/defaulttese.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/defaultfrag.spv"};
		VKSubInitPipeline(&GraphicsHandler::vulkaninfo.primarygraphicspipeline,
						  VK_SHADER_STAGE_VERTEX_BIT
						  | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
						  | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
						  | VK_SHADER_STAGE_FRAGMENT_BIT,
						  &shaderfilepathstemp[0],
						  &descsetlayoutcreateinfos[0],
						  {VK_SHADER_STAGE_VERTEX_BIT
						   | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
						   | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
						   | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PrimaryGraphicsPushConstants)},
						  {
								  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
								  nullptr,
								  0,
								  1,
								  &vertinbindingdesc,
								  3,
								  &vertinattribdesc[0]
						  },
						  true);
	}
	//text
	{
		VkDescriptorSetLayoutBinding objectdslbinding {
				0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				nullptr
		};
		VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {{
																   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
																   nullptr,
																   0,
																   0,
																   nullptr
														   },
														   {
																   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
																   nullptr,
																   0,
																   1,
																   &objectdslbinding
														   }};
		const char* shaderfilepathstemp[2] = {WORKING_DIRECTORY "/resources/shaders/SPIRV/textvert.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/textfrag.spv"};
		VKSubInitPipeline(&GraphicsHandler::vulkaninfo.textgraphicspipeline,
						  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						  &shaderfilepathstemp[0],
						  &dslcreateinfos[0],
						  {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TextPushConstants)},
						  {
								  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
								  nullptr,
								  0,
								  0,
								  nullptr,
								  0,
								  nullptr
						  },
						  false);
	}
	//skybox
	{
		VkDescriptorSetLayoutBinding binding[1] {{
														 0,
														 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
														 1,
														 VK_SHADER_STAGE_FRAGMENT_BIT,
														 nullptr
												 }};
		VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {
				{      //ideally migrate this to either all scenedsl, or no scenedsl and all objectdsl
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						1,
						&binding[0]
				},
				{
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						0,
						nullptr
				}};
		const char* shaderfilepathstemp[2] = {WORKING_DIRECTORY "/resources/shaders/SPIRV/skyboxvert.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/skyboxfrag.spv"};
		VKSubInitPipeline(&GraphicsHandler::vulkaninfo.skyboxgraphicspipeline,
						  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						  &shaderfilepathstemp[0],
						  &dslcreateinfos[0],
						  {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPushConstants)},
						  {
								  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
								  nullptr,
								  0,
								  0,
								  nullptr,
								  0,
								  nullptr
						  },
						  false);
	}
	//ocean graph
	{
		VkDescriptorSetLayoutBinding objectdslbindings[3] {{
																   0,
																   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
																   1,
																   VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
																   nullptr
														   },
														   {
																   1,
																   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
																   1,
																   VK_SHADER_STAGE_FRAGMENT_BIT,
																   nullptr
														   },
														   {
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
		VkVertexInputAttributeDescription vertinattribdesc[3] {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,
																										   position)},
															   {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,
																										   normal)},
															   {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex,
																										uv)}};
		const char* shaderfilepathstemp[4] = {WORKING_DIRECTORY "/resources/shaders/SPIRV/oceanvert.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/oceantesc.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/oceantese.spv",
											  WORKING_DIRECTORY "/resources/shaders/SPIRV/oceanfrag.spv"};
		VKSubInitPipeline(&vulkaninfo.oceangraphicspipeline,
						  VK_SHADER_STAGE_VERTEX_BIT
						  | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
						  | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
						  | VK_SHADER_STAGE_FRAGMENT_BIT,
						  &shaderfilepathstemp[0],
						  &dslcreateinfos[0],
						  {VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
						   | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
						   | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(OceanPushConstants)},
						  {
								  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
								  nullptr,
								  0,
								  1,
								  &vertinbindingdesc,
								  3,
								  &vertinattribdesc[0]
						  },
						  true);
	}
	//ocean comp
	{
		VkDescriptorSetLayoutBinding objdslbindings[2] {{
																0,
																VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
																1,
																VK_SHADER_STAGE_COMPUTE_BIT,
																nullptr
														},
														{
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
														   },
														   {
																   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
																   nullptr,
																   0,
																   2,
																   &objdslbindings[0]
														   }};
		const char* shaderfilepaths[1] = {WORKING_DIRECTORY "/resources/shaders/SPIRV/oceancomp.spv"};
		VKSubInitPipeline(&vulkaninfo.oceancomputepipeline,
						  VK_SHADER_STAGE_COMPUTE_BIT,
						  &shaderfilepaths[0],
						  &dslcreateinfos[0],
						  {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)},
						  {
								  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
								  nullptr,
								  0,
								  0,
								  nullptr,
								  0,
								  nullptr
						  },
						  false);
	}
	//grass (more like gross lol)
	{
		VkDescriptorSetLayoutBinding objectbinding {
				0,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				1,
				VK_SHADER_STAGE_VERTEX_BIT,
				nullptr
		};
		VkDescriptorSetLayoutCreateInfo descsetlayoutcreateinfos[2] {
//			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//			nullptr,
//			0,
//			0,
//			nullptr
//		}, {
				scenedslcreateinfo, {
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						1,
						&objectbinding
				}};
		VkVertexInputBindingDescription vertinbindingdescs {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
		VkVertexInputAttributeDescription vertinattribdescs[2] {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
				{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}};
		const char* shaderfilepaths[4] {WORKING_DIRECTORY "/resources/shaders/SPIRV/grassvert.spv",
										WORKING_DIRECTORY "/resources/shaders/SPIRV/grasstesc.spv",
										WORKING_DIRECTORY "/resources/shaders/SPIRV/grasstese.spv",
										WORKING_DIRECTORY "/resources/shaders/SPIRV/grassfrag.spv"};
		VKSubInitPipeline(
				&vulkaninfo.grassgraphicspipeline,
				VK_SHADER_STAGE_VERTEX_BIT
				| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
				| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
				| VK_SHADER_STAGE_FRAGMENT_BIT,
				&shaderfilepaths[0],
				&descsetlayoutcreateinfos[0],
				{
						VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
						0,
						sizeof(glm::mat4)
				},
				{
						VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
						nullptr,
						0,
						1,
						&vertinbindingdescs,
						2,
						&vertinattribdescs[0]
				},
				true);
	}
	//shadowmap
	{
		VkDescriptorSetLayoutBinding objecttexdslbindings[5] {
				{
						0,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						1,
						VK_SHADER_STAGE_VERTEX_BIT,
						nullptr
				},
				{
						1,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						1,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				},
				{
						2,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						1,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				},
				{
						3,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						1,
						VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				},
				{
						4,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						MAX_LIGHTS,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						nullptr
				}};
		VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfos[2] {  //perhaps this much superfluous info is inefficient???
				scenedslcreateinfo,
				{
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						5,
						&objecttexdslbindings[0]
				}
		};
		VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
		VkVertexInputAttributeDescription vertinattribdesc {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
															offsetof(Vertex, position)};
		const char* shaderfilepaths[2] {WORKING_DIRECTORY "/resources/shaders/SPIRV/shadowmapvert.spv",
										WORKING_DIRECTORY "/resources/shaders/SPIRV/shadowmapfrag.spv"};
		VKSubInitPipeline(
				&vulkaninfo.shadowmapgraphicspipeline,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				&shaderfilepaths[0],
				&descriptorsetlayoutcreateinfos[0],
				{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowmapPushConstants)},
				{
						VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
						nullptr,
						0,
						1,
						&vertinbindingdesc,
						1,
						&vertinattribdesc
				},
				true);
	}
	//texmon
	{
		VkDescriptorSetLayoutBinding descriptorsetlayoutbinding {
				0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				nullptr
		};
		VkDescriptorSetLayoutCreateInfo descriptorsetlayoutcreateinfos[2] {
				{
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						0,
						nullptr
				},
				{
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						1,
						&descriptorsetlayoutbinding
				}};
		const char* shaderfilepaths[2] {WORKING_DIRECTORY "/resources/shaders/SPIRV/texmonvert.spv",
										WORKING_DIRECTORY "/resources/shaders/SPIRV/texmonfrag.spv"};
		VKSubInitPipeline(
				&GraphicsHandler::vulkaninfo.texmongraphicspipeline,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				&shaderfilepaths[0],
				&descriptorsetlayoutcreateinfos[0],
				{},
				{
						VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
						nullptr,
						0,
						0,
						nullptr,
						0,
						nullptr
				},
				false);
	}
	//lines
	{
		VkVertexInputBindingDescription vibd {
				0,
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX
		};
		VkVertexInputAttributeDescription viad {
				0,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0
		};
		VkPipelineVertexInputStateCreateInfo pvisci {
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				nullptr,
				0,
				1,
				&vibd,
				1,
				&viad
		};
		VkDescriptorSetLayoutCreateInfo dslci[2] {
				scenedslcreateinfo,
				{
						VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						nullptr,
						0,
						0,
						nullptr
				}};
		const char* shaderfilepaths[2] {
				WORKING_DIRECTORY "/resources/shaders/SPIRV/linevert.spv",
				WORKING_DIRECTORY "/resources/shaders/SPIRV/linefrag.spv"
		};
		VKSubInitPipeline(
				&GraphicsHandler::vulkaninfo.linegraphicspipeline,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				&shaderfilepaths[0],
				&dslci[0],
				{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)},
				pvisci,
				true);
	}
}

void GraphicsHandler::VKSubInitWindow () {
	KeyInfo infotemp = {GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN, false, false};
	keyvalues.emplace(GLFW_KEY_W, infotemp);
	keyvalues.emplace(GLFW_KEY_A, infotemp);
	keyvalues.emplace(GLFW_KEY_S, infotemp);
	keyvalues.emplace(GLFW_KEY_D, infotemp);
	std::cout << (glfwInit() == GLFW_TRUE ? "" : "glfw failed to init\n");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	vulkaninfo.width = mode->width;
	vulkaninfo.height = mode->height;
	vulkaninfo.window = glfwCreateWindow(vulkaninfo.width, vulkaninfo.height, "Vulkan Sandbox", nullptr, nullptr);
	glfwGetFramebufferSize(vulkaninfo.window, &vulkaninfo.horizontalres, &vulkaninfo.verticalres);
	glfwMakeContextCurrent(vulkaninfo.window);
	glfwSetInputMode(vulkaninfo.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(vulkaninfo.window, keystrokeCallback);
	glfwSetErrorCallback(glfwErrorCallback);
}

void GraphicsHandler::VKSubInitInstance () {
	VkApplicationInfo appinfo {
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,
			"Vulkan Sandbox",
			VK_MAKE_VERSION(1, 0, 0),
			"Jet",
			VK_MAKE_VERSION(1, 0, 0),
			VK_MAKE_API_VERSION(1, 1, 0, 0)
	};
	const char* layernames[1] = {"VK_LAYER_KHRONOS_validation"};

	uint32_t instanceextcount = -1u;
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceextcount, nullptr);
	VkExtensionProperties instanceextprops[instanceextcount];
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceextcount, &instanceextprops[0]);
	char* instanceextnames[instanceextcount];
	for (uint32_t x = 0; x < instanceextcount; x++) {
		instanceextnames[x] = instanceextprops[x].extensionName;
//		std::cout<<"just added ext number "<<x<<", name is "<<instanceextnames[x]<<std::endl;
	}

	VkInstanceCreateInfo instancecreateinfo {
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			nullptr,
			0,
			&appinfo,
			1,
			&layernames[0],
//			0, nullptr,
			instanceextcount,
			&instanceextnames[0]
	};
	VkValidationFeaturesEXT validationfeatures {
			VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
			instancecreateinfo.pNext,
			0,
			nullptr,
			0,
			nullptr
	};
	instancecreateinfo.pNext = &validationfeatures;
	vkCreateInstance(&instancecreateinfo, nullptr, &vulkaninfo.instance);
	glfwCreateWindowSurface(vulkaninfo.instance, vulkaninfo.window, nullptr, &vulkaninfo.surface);
}

void GraphicsHandler::VKSubInitDebug () {
	VkDebugUtilsMessengerCreateInfoEXT messengercreateinfo {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			nullptr,
			0,
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			debugCallback,
			nullptr
	};
	CreateDebugUtilsMessengerEXT(vulkaninfo.instance, &messengercreateinfo, nullptr, &vulkaninfo.debugmessenger);
}

void GraphicsHandler::VKSubInitDevices () {
	uint32_t physicaldevicecount = 1, queuefamilycount = -1u, deviceextcount = -1u;
	vkEnumeratePhysicalDevices(vulkaninfo.instance, &physicaldevicecount, &vulkaninfo.physicaldevice);
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, nullptr);
	VkQueueFamilyProperties queuefamilyprops[queuefamilycount];
	vkGetPhysicalDeviceQueueFamilyProperties(vulkaninfo.physicaldevice, &queuefamilycount, &queuefamilyprops[0]);

	vulkaninfo.graphicsqueuefamilyindex = -1u;
	vulkaninfo.presentqueuefamilyindex = -1u;
	vulkaninfo.transferqueuefamilyindex = -1u;
	VkBool32 supportssurfacepresentation;
	for (uint32_t x = 0; x < queuefamilycount; x++) {
		if ((queuefamilyprops[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			vulkaninfo.graphicsqueuefamilyindex == -1u)
			vulkaninfo.graphicsqueuefamilyindex = x;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkaninfo.physicaldevice, x, vulkaninfo.surface,
											 &supportssurfacepresentation);
		if (supportssurfacepresentation && vulkaninfo.presentqueuefamilyindex == -1u)
			vulkaninfo.presentqueuefamilyindex = x;
		if ((queuefamilyprops[x].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
			vulkaninfo.transferqueuefamilyindex == -1u)
			vulkaninfo.transferqueuefamilyindex = x;
	}
	std::vector<VkDeviceQueueCreateInfo> queuecreateinfos;
	float queuepriority = 1.0f;
	queuecreateinfos.push_back({
									   VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
									   nullptr,
									   0,
									   vulkaninfo.graphicsqueuefamilyindex,
									   1,
									   &queuepriority
							   });
	if (vulkaninfo.graphicsqueuefamilyindex != vulkaninfo.presentqueuefamilyindex) {
		queuecreateinfos.push_back({
										   VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										   nullptr,
										   0,
										   vulkaninfo.presentqueuefamilyindex,
										   1,
										   &queuepriority
								   });
	}
	if (vulkaninfo.graphicsqueuefamilyindex != vulkaninfo.transferqueuefamilyindex &&
		vulkaninfo.presentqueuefamilyindex != vulkaninfo.transferqueuefamilyindex) {
		queuecreateinfos.push_back({
										   VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										   nullptr,
										   0,
										   vulkaninfo.transferqueuefamilyindex,
										   1,
										   &queuepriority
								   });
	}
	vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextcount, nullptr);
	VkExtensionProperties deviceextprops[deviceextcount];
	vkEnumerateDeviceExtensionProperties(vulkaninfo.physicaldevice, nullptr, &deviceextcount, &deviceextprops[0]);
	std::vector<const char*> deviceextnames;
	for (uint32_t x = 0; x < deviceextcount; x++) {
//		std::cout<<deviceextprops[x].extensionName<<std::endl;
		if (strcmp(deviceextprops[x].extensionName, "VK_KHR_portability_subset"))
			deviceextnames.push_back("VK_KHR_portability_subset");
		if (strcmp(deviceextprops[x].extensionName, "VK_KHR_swapchain")) deviceextnames.push_back("VK_KHR_swapchain");
	}
	VkPhysicalDeviceFeatures feattemp {};
	vkGetPhysicalDeviceFeatures(vulkaninfo.physicaldevice, &feattemp);
	VkPhysicalDeviceFeatures physicaldevicefeatures {};
	physicaldevicefeatures.tessellationShader = VK_TRUE;
	physicaldevicefeatures.fillModeNonSolid = VK_TRUE;
	VkPhysicalDeviceProperties pdprops;
	vkGetPhysicalDeviceProperties(vulkaninfo.physicaldevice, &pdprops);
	//dont forget about anisotropy
	//could include all available, but for now will include none
	VkPhysicalDevicePortabilitySubsetFeaturesKHR portsubfeats {};
	portsubfeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
	portsubfeats.pNext = nullptr;
	portsubfeats.mutableComparisonSamplers = VK_TRUE;
	VkDeviceCreateInfo devicecreateinfo {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			&portsubfeats,
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

void GraphicsHandler::VKSubInitQueues () {
	vkGetDeviceQueue(vulkaninfo.logicaldevice, vulkaninfo.graphicsqueuefamilyindex, 0, &vulkaninfo.graphicsqueue);
	vkGetDeviceQueue(vulkaninfo.logicaldevice, vulkaninfo.presentqueuefamilyindex, 0, &vulkaninfo.presentationqueue);
}

void GraphicsHandler::VKSubInitSwapchain () {
	VkSurfaceCapabilitiesKHR surfacecaps {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkaninfo.physicaldevice, vulkaninfo.surface, &surfacecaps);
	vulkaninfo.swapchainextent = surfacecaps.currentExtent;
	vulkaninfo.numswapchainimages = surfacecaps.minImageCount + 1;
	bool queuefamilyindicesaresame = vulkaninfo.graphicsqueuefamilyindex == vulkaninfo.presentqueuefamilyindex;
	VkSwapchainCreateInfoKHR swapchaincreateinfo {
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			nullptr,
			0,
			vulkaninfo.surface,
			vulkaninfo.numswapchainimages,
			VK_FORMAT_B8G8R8A8_SRGB,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
			vulkaninfo.swapchainextent,
			1,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			queuefamilyindicesaresame ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			queuefamilyindicesaresame ? 0u : 2u,
			queuefamilyindicesaresame ? nullptr : &vulkaninfo.graphicsqueuefamilyindex,
			surfacecaps.currentTransform,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_PRESENT_MODE_FIFO_KHR,
			VK_TRUE,
			VK_NULL_HANDLE
	};
	vkCreateSwapchainKHR(vulkaninfo.logicaldevice, &swapchaincreateinfo, nullptr, &vulkaninfo.swapchain);
	uint32_t imgcounttemp = -1u;
	vkGetSwapchainImagesKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, &imgcounttemp, nullptr);
	vulkaninfo.swapchainimages = new VkImage[vulkaninfo.numswapchainimages];
	vkGetSwapchainImagesKHR(vulkaninfo.logicaldevice, vulkaninfo.swapchain, &vulkaninfo.numswapchainimages,
							&vulkaninfo.swapchainimages[0]);
	vulkaninfo.swapchainimageviews = new VkImageView[vulkaninfo.numswapchainimages];
	changeflags = new ChangeFlag[vulkaninfo.numswapchainimages];
	for (uint32_t x = 0; x < vulkaninfo.numswapchainimages; x++) {
		// TODO: move this to a more appropriate place
		changeflags[x] = NO_CHANGE_FLAG_BIT;
		VkImageViewCreateInfo imageviewcreateinfo {
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				nullptr,
				0,
				vulkaninfo.swapchainimages[x],
				VK_IMAGE_VIEW_TYPE_2D,
				SWAPCHAIN_IMAGE_FORMAT,
				{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
				 VK_COMPONENT_SWIZZLE_IDENTITY},
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};
		vkCreateImageView(vulkaninfo.logicaldevice, &imageviewcreateinfo, nullptr, &vulkaninfo.swapchainimageviews[x]);
	}
	swapchainimageindex = 0u;
}

void GraphicsHandler::VKSubInitRenderpasses () {
	// shadowmap renderpass (template)
	VkAttachmentDescription shadowattachmentdescription {
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
	VkAttachmentReference shadowattachmentreference {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	VkSubpassDescription shadowsubpassdescription {
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			nullptr,
			0,
			nullptr,
			nullptr,
			&shadowattachmentreference,
			0,
			nullptr
	};
	VkSubpassDependency shadowsubpassdependencies[2] {{
															  VK_SUBPASS_EXTERNAL,
															  0,
															  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
															  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
															  VK_ACCESS_SHADER_READ_BIT,
															  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
															  0
													  },
													  {
															  0,
															  VK_SUBPASS_EXTERNAL,
															  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
															  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
															  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
															  VK_ACCESS_SHADER_READ_BIT,
															  0
													  }};
	VkRenderPassCreateInfo shadowrenderpasscreateinfo {
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			nullptr,
			0,
			1,
			&shadowattachmentdescription,
			1,
			&shadowsubpassdescription,
			2,
			&shadowsubpassdependencies[0]
	};
	vkCreateRenderPass(vulkaninfo.logicaldevice, &shadowrenderpasscreateinfo, nullptr,
					   &vulkaninfo.templateshadowrenderpass);

	// primary renderpass
	VkAttachmentDescription prattachmentdescriptions[2] {{
																 0,
																 SWAPCHAIN_IMAGE_FORMAT,
																 VK_SAMPLE_COUNT_1_BIT,
																 VK_ATTACHMENT_LOAD_OP_CLEAR,
																 VK_ATTACHMENT_STORE_OP_STORE,
																 VK_ATTACHMENT_LOAD_OP_DONT_CARE,
																 VK_ATTACHMENT_STORE_OP_DONT_CARE,
																 VK_IMAGE_LAYOUT_UNDEFINED,
																 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
														 },
														 {
																 0,
																 VK_FORMAT_D32_SFLOAT,
																 VK_SAMPLE_COUNT_1_BIT,
																 VK_ATTACHMENT_LOAD_OP_CLEAR,
																 VK_ATTACHMENT_STORE_OP_STORE,
																 VK_ATTACHMENT_LOAD_OP_DONT_CARE,
																 VK_ATTACHMENT_STORE_OP_DONT_CARE,
																 VK_IMAGE_LAYOUT_UNDEFINED,
																 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
														 }};
	VkAttachmentReference prcolorattachmentreference {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			prdepthattachmentreference {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	VkSubpassDescription prsubpass {
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,
			1,
			&prcolorattachmentreference,
			nullptr,
			&prdepthattachmentreference,
			0, nullptr
	};
	VkSubpassDependency prsubpassdepenedency {
			VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			0
	};

	VkRenderPassCreateInfo primaryrenderpasscreateinfo {
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			nullptr,
			0,
			2, &prattachmentdescriptions[0],
			1, &prsubpass,
			1, &prsubpassdepenedency    // can this just be nullptr???
	};
	vkCreateRenderPass(vulkaninfo.logicaldevice, &primaryrenderpasscreateinfo, nullptr, &vulkaninfo.primaryrenderpass);
}

void GraphicsHandler::VKSubInitDescriptorLayoutsAndPool (uint32_t nummeshes) {
	VkDescriptorSetLayoutBinding scenedslbindings[2] {{
															  0,
															  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
															  MAX_LIGHTS,
															  VK_SHADER_STAGE_FRAGMENT_BIT |
															  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
															  nullptr
													  },
													  {
															  1,
															  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
															  1,
															  VK_SHADER_STAGE_FRAGMENT_BIT,
															  nullptr
													  }};
	VkDescriptorSetLayoutCreateInfo scenedslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			2,
			&scenedslbindings[0]
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &scenedslcreateinfo, nullptr, &vulkaninfo.scenedsl);

	VkDescriptorSetLayoutBinding defaultdslbindings[5] {
			{
					0,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					1,
					VK_SHADER_STAGE_VERTEX_BIT,
					nullptr
			},
			{
					1,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			},
			{
					2,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			},
			{
					3,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			},
			{
					4,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					MAX_LIGHTS,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			}};
	VkDescriptorSetLayoutCreateInfo defaultdslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			5, &defaultdslbindings[0]
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &defaultdslcreateinfo, nullptr, &vulkaninfo.defaultdsl);

	VkDescriptorSetLayoutBinding textdslbinding {
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
	};
	VkDescriptorSetLayoutCreateInfo textdslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1, &textdslbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &textdslcreateinfo, nullptr, &vulkaninfo.textdsl);

	VkDescriptorSetLayoutBinding oceangraphdslbindings[2] {{
																   0,
																   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
																   1,
																   VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
																   nullptr
														   },
														   {
																   1,
																   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
																   1,
																   VK_SHADER_STAGE_FRAGMENT_BIT,
																   nullptr
														   }};
	VkDescriptorSetLayoutCreateInfo oceangraphdslcreateinfos {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			2, &oceangraphdslbindings[0]
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &oceangraphdslcreateinfos, nullptr,
								&vulkaninfo.oceangraphdsl);

	VkDescriptorSetLayoutBinding oceancompdslbindings[2] {{
																  0,
																  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
																  1,
																  VK_SHADER_STAGE_COMPUTE_BIT,
																  nullptr
														  },
														  {
																  1,
																  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
																  1,
																  VK_SHADER_STAGE_COMPUTE_BIT,
																  nullptr
														  }};
	VkDescriptorSetLayoutCreateInfo oceancompdslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			2, &oceancompdslbindings[0]
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &oceancompdslcreateinfo, nullptr, &vulkaninfo.oceancompdsl);

	VkDescriptorSetLayoutBinding particledslbinding {
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			nullptr
	};
	VkDescriptorSetLayoutCreateInfo particledslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1, &particledslbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &particledslcreateinfo, nullptr, &vulkaninfo.particledsl);

	VkDescriptorSetLayoutBinding shadowmapdslbindings[5] {
			{
					0,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					1,
					VK_SHADER_STAGE_VERTEX_BIT,
					nullptr
			},
			{
					1,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			},
			{
					2,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			},
			{
					3,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			},
			{
					4,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					MAX_LIGHTS,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					nullptr
			}};
	VkDescriptorSetLayoutCreateInfo shadowmapdslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			5, &shadowmapdslbindings[0]
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &shadowmapdslcreateinfo, nullptr, &vulkaninfo.shadowmapdsl);

	VkDescriptorSetLayoutBinding texmondslbinding {
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			nullptr
	};
	VkDescriptorSetLayoutCreateInfo texmondslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1, &texmondslbinding
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &texmondslcreateinfo, nullptr, &vulkaninfo.texmondsl);

	VkDescriptorSetLayoutCreateInfo linesdslcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			0,
			nullptr
	};
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice, &linesdslcreateinfo, nullptr, &vulkaninfo.linesdsl);

	//one ubo per light
	//three imgsamplers per mesh, one for text, two for ocean
	//two storageimg for ocean
	//one for particle system, plus 3 auto-init'd by mesh parent
	//one input attachment for ocean, two for compositing
	//we should find a better way to do this lol
	VkDescriptorPoolSize descriptorpoolsizes[5] {{
														 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
														 vulkaninfo.numswapchainimages *
														 (2 * MAX_LIGHTS + nummeshes)
												 },
												 {
														 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
														 vulkaninfo.numswapchainimages *
														 (4 * nummeshes + 2 + 1 + 1 + 1 + 1 + 1) + MAX_FRAMES_IN_FLIGHT
												 },
												 {
														 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
														 vulkaninfo.numswapchainimages * 2
												 },
												 {
														 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
														 vulkaninfo.numswapchainimages
												 },
												 {
														 VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
														 vulkaninfo.numswapchainimages * (1 + 2)
												 }};
	VkDescriptorPoolCreateInfo descriptorpoolcreateinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			nullptr,
			VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,      //perhaps look into flags; do we really need update after bind?
			vulkaninfo.numswapchainimages * (2 * MAX_LIGHTS + 4 * nummeshes + 2 + 1 + 1 + 1 + 1 + 1 + 2) +
			MAX_FRAMES_IN_FLIGHT,
			5,
			&descriptorpoolsizes[0]
	};
	vkCreateDescriptorPool(vulkaninfo.logicaldevice, &descriptorpoolcreateinfo, nullptr, &vulkaninfo.descriptorpool);
}

void GraphicsHandler::VKSubInitUniformBuffer (
		VkDescriptorSet** descsets,
		uint32_t binding,
		VkDeviceSize elementsize,
		uint32_t elementcount,
		VkBuffer** buffers,
		VkDeviceMemory** memories,
		VkDescriptorSetLayout descsetlayout) {
	// TODO: don't do this descriptor set alloc. this is awful. this was a terrible idea, why did you ever do this???
	if (*descsets) {
		vkFreeDescriptorSets(vulkaninfo.logicaldevice,
							 vulkaninfo.descriptorpool,
							 vulkaninfo.numswapchainimages,
							 *descsets);
		delete[] *descsets;
	}
	*descsets = new VkDescriptorSet[vulkaninfo.numswapchainimages];       //is this really something we should be doing inside the method???
	VkDescriptorSetLayout dsltemp[vulkaninfo.numswapchainimages];
	for (unsigned char x = 0; x < vulkaninfo.numswapchainimages; x++) dsltemp[x] = descsetlayout;
	VkDescriptorSetAllocateInfo descsetallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			vulkaninfo.descriptorpool,
			vulkaninfo.numswapchainimages,
			&dsltemp[0]
	};
	vkAllocateDescriptorSets(vulkaninfo.logicaldevice, &descsetallocinfo, *descsets);

	*buffers = new VkBuffer[vulkaninfo.numswapchainimages];   //kinda dangerous to do this dynamic allocation within function
	*memories = new VkDeviceMemory[vulkaninfo.numswapchainimages];

	VkPhysicalDeviceProperties pdprops;
	vkGetPhysicalDeviceProperties(vulkaninfo.physicaldevice, &pdprops);
	VkDeviceSize roundedupsize = elementsize;
	if (elementsize % pdprops.limits.minUniformBufferOffsetAlignment != 0) {
		roundedupsize = (1 + floor((float)elementsize / (float)pdprops.limits.minUniformBufferOffsetAlignment)) *
						pdprops.limits.minUniformBufferOffsetAlignment;
	}

	for (uint32_t x = 0; x < vulkaninfo.numswapchainimages; x++) {
		//why arent we using VKHelperCreateAndAllocateBuffer?
		VkBufferCreateInfo buffcreateinfo {
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				nullptr,
				0,
				roundedupsize * elementcount,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_SHARING_MODE_EXCLUSIVE,
				0,
				nullptr
		};
		vkCreateBuffer(vulkaninfo.logicaldevice, &buffcreateinfo, nullptr, &((*buffers)[x]));
		VkMemoryRequirements memrequirements {};
		vkGetBufferMemoryRequirements(vulkaninfo.logicaldevice, (*buffers)[x], &memrequirements);
		VkPhysicalDeviceMemoryProperties physicaldevicememprops {};
		vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
		uint32_t memindex = -1u;
		for (uint32_t x = 0; x < physicaldevicememprops.memoryTypeCount; x++) {
			if (memrequirements.memoryTypeBits & (1 << x)
				&& physicaldevicememprops.memoryTypes[x].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				&& physicaldevicememprops.memoryTypes[x].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
				memindex = x;
				break;
			}
		}
		VkMemoryAllocateInfo memallocateinfo {
				VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				nullptr,
				memrequirements.size,
				memindex
		};
		vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, &(*memories)[x]);
		vkBindBufferMemory(vulkaninfo.logicaldevice, (*buffers)[x], (*memories)[x], 0);

		VkDescriptorBufferInfo descriptorbuffinfos[elementcount];
		for (uint32_t y = 0; y < elementcount; y++) {
			descriptorbuffinfos[y].buffer = (*buffers)[x];
			descriptorbuffinfos[y].offset = y * roundedupsize;
			descriptorbuffinfos[y].range = elementsize;
		}
		VkWriteDescriptorSet writedescriptorset {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				(*descsets)[x],
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

void GraphicsHandler::VKSubInitStorageBuffer (
		VkDescriptorSet** descriptorsets,
		uint32_t binding,
		VkDeviceSize elementsize,
		uint32_t elementcount,
		VkBuffer** buffers,
		VkDeviceMemory** memories) {
	VkPhysicalDeviceProperties pdprops;
	vkGetPhysicalDeviceProperties(vulkaninfo.physicaldevice, &pdprops);
	VkDeviceSize roundedupsize = elementsize;
	if (elementsize % pdprops.limits.minUniformBufferOffsetAlignment != 0) {
		roundedupsize = (1 + floor((float)elementsize / (float)pdprops.limits.minUniformBufferOffsetAlignment)) *
						pdprops.limits.minUniformBufferOffsetAlignment;
	}
	for (uint8_t scii = 0; scii < GraphicsHandler::vulkaninfo.numswapchainimages; scii++) {
		VKHelperCreateAndAllocateBuffer(
				&buffers[0][scii],
				roundedupsize * elementcount,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				&memories[0][scii],
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//unneccesary, clean up later
		VkDescriptorBufferInfo descriptorbuffinfos[elementcount];
		for (uint32_t y = 0; y < elementcount; y++) {
			descriptorbuffinfos[y].buffer = (*buffers)[scii];
			descriptorbuffinfos[y].offset = y * roundedupsize;
			descriptorbuffinfos[y].range = elementsize;
		}
		//clearly my understanding of this struct, esp the elementcount field, is flawed
		VkWriteDescriptorSet writedescriptorset {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				(*descriptorsets)[scii],
				binding,
				0,
				1,      //i mean, we only /really/ have to update the buffers for active lights
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				nullptr,
				&descriptorbuffinfos[0],
				nullptr
		};
		vkUpdateDescriptorSets(vulkaninfo.logicaldevice, 1, &writedescriptorset, 0, nullptr);
	}
}

void GraphicsHandler::VKSubInitFramebuffers () {
	vulkaninfo.primaryframebuffers = new VkFramebuffer[vulkaninfo.numswapchainimages];
	for (uint8_t x = 0; x < vulkaninfo.numswapchainimages; x++) {
		VkImageView attachmentstemp[2] = {vulkaninfo.swapchainimageviews[x],
										  vulkaninfo.depthbuffer.imageview};
		VkFramebufferCreateInfo framebuffercreateinfo {
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				nullptr,
				0,
				vulkaninfo.primaryrenderpass,
				2, &attachmentstemp[0],
				vulkaninfo.swapchainextent.width, vulkaninfo.swapchainextent.height, 1
		};
		vkCreateFramebuffer(vulkaninfo.logicaldevice, &framebuffercreateinfo, nullptr,
							&vulkaninfo.primaryframebuffers[x]);
	}
}

void GraphicsHandler::VKSubInitCommandPool () {
	VkCommandPoolCreateInfo cmdpoolcreateinfo {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			vulkaninfo.graphicsqueuefamilyindex
	};
	vkCreateCommandPool(vulkaninfo.logicaldevice, &cmdpoolcreateinfo, nullptr, &vulkaninfo.commandpool);
	VkCommandBufferAllocateInfo cmdbufferallocateinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			vulkaninfo.commandpool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			MAX_FRAMES_IN_FLIGHT
	};
	vulkaninfo.commandbuffers = new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
	vkAllocateCommandBuffers(vulkaninfo.logicaldevice, &cmdbufferallocateinfo, &vulkaninfo.commandbuffers[0]);
	cmdbufferallocateinfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(vulkaninfo.logicaldevice, &cmdbufferallocateinfo, &(vulkaninfo.interimcommandbuffer));
	vulkaninfo.primaryclears[0] = {0.0f, 0.0f, 0.1f, 1.0f};
	vulkaninfo.primaryclears[1] = {1.0f, 0.0f};
	vulkaninfo.shadowmapclear = {1.f, 0.f};

	for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		for (uint8_t t = 0; t < NUM_RECORDING_THREADS; t++) {
			vkCreateCommandPool(vulkaninfo.logicaldevice,
								&cmdpoolcreateinfo,
								nullptr,
								&vulkaninfo.threadCbSets[t][i].pool);
		}
	}
}

void GraphicsHandler::VKSubInitSemaphoresAndFences () {
	vulkaninfo.currentframeinflight = 0;
	VkSemaphoreCreateInfo semaphorecreateinfo {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,
			0
	};
	VkFenceCreateInfo fencecreateinfo {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			nullptr,
			VK_FENCE_CREATE_SIGNALED_BIT
	};
	//eventually move to statically size array instead of ptr
	vulkaninfo.submitfinishedfences = new VkFence[MAX_FRAMES_IN_FLIGHT];
	vulkaninfo.presentfinishedfences = new VkFence[MAX_FRAMES_IN_FLIGHT];
	for (int x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphorecreateinfo, nullptr,
						  &vulkaninfo.imageavailablesemaphores[x]);
		vkCreateSemaphore(vulkaninfo.logicaldevice, &semaphorecreateinfo, nullptr,
						  &vulkaninfo.renderfinishedsemaphores[x]);
		vkCreateFence(vulkaninfo.logicaldevice, &fencecreateinfo, nullptr, &vulkaninfo.submitfinishedfences[x]);
		vkCreateFence(vulkaninfo.logicaldevice, &fencecreateinfo, nullptr, &vulkaninfo.presentfinishedfences[x]);
//		vkCreateFence(vulkaninfo.logicaldevice, &fencecreateinfo, nullptr, &vulkaninfo.frameinflightfences[x]);
	}
	vulkaninfo.recordinginvalidfences = new VkFence[vulkaninfo.numswapchainimages];
	for (int x = 0; x < vulkaninfo.numswapchainimages; x++) {
//		vulkaninfo.imageinflightfences[x]=VK_NULL_HANDLE;
		vkCreateFence(vulkaninfo.logicaldevice, &fencecreateinfo, nullptr, &vulkaninfo.recordinginvalidfences[x]);
	}
}

void GraphicsHandler::VKSubInitSamplers () {
	VkSamplerCreateInfo samplercreateinfo {
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			nullptr,
			0,
			VK_FILTER_NEAREST,
			VK_FILTER_NEAREST,
			VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			0.0,
			VK_FALSE,
			1.0,
			VK_FALSE,
			VK_COMPARE_OP_LESS,
			0.0,
			0.0,
			VK_BORDER_COLOR_INT_OPAQUE_WHITE,
			VK_FALSE
	};
	vkCreateSampler(vulkaninfo.logicaldevice, &samplercreateinfo, nullptr, &genericsampler);
	samplercreateinfo.minFilter = VK_FILTER_LINEAR;
	samplercreateinfo.magFilter = VK_FILTER_LINEAR;
	samplercreateinfo.compareEnable = VK_TRUE;
	vkCreateSampler(vulkaninfo.logicaldevice, &samplercreateinfo, nullptr, &depthsampler);
	samplercreateinfo.minFilter = VK_FILTER_NEAREST;
	samplercreateinfo.magFilter = VK_FILTER_NEAREST;
	samplercreateinfo.compareEnable = VK_FALSE;
	samplercreateinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplercreateinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplercreateinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	vkCreateSampler(vulkaninfo.logicaldevice, &samplercreateinfo, nullptr, &clamptobordersampler);
	samplercreateinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplercreateinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplercreateinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplercreateinfo.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(vulkaninfo.logicaldevice, &samplercreateinfo, nullptr, &linearminsampler);
	samplercreateinfo.magFilter = VK_FILTER_LINEAR;
	vkCreateSampler(vulkaninfo.logicaldevice, &samplercreateinfo, nullptr, &linearminmagsampler);
}

void GraphicsHandler::VKSubInitPipeline (
		PipelineInfo* pipelineinfo,
		VkShaderStageFlags shaderstages,
		const char** shaderfilepaths,
		VkDescriptorSetLayoutCreateInfo* descsetlayoutcreateinfos,
		VkPushConstantRange pushconstantrange,
		VkPipelineVertexInputStateCreateInfo vertexinputstatecreateinfo,
		bool depthtest) {
	if (shaderstages & VK_SHADER_STAGE_COMPUTE_BIT) {
		vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice,
									&descsetlayoutcreateinfos[0],
									nullptr,
									&pipelineinfo->scenedsl);
		vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice,
									&descsetlayoutcreateinfos[1],
									nullptr,
									&pipelineinfo->objectdsl);
		VkDescriptorSetLayout descsetlayoutstemp[2] = {pipelineinfo->scenedsl, pipelineinfo->objectdsl};
		bool scenedslexists = descsetlayoutcreateinfos[0].bindingCount != 0u, objectdslexists =
				descsetlayoutcreateinfos[1].bindingCount != 0u;
		VkPipelineLayoutCreateInfo pipelinelayoutcreateinfo {
				VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				nullptr,
				0,
				(uint32_t)scenedslexists + (uint32_t)objectdslexists,
				(scenedslexists | objectdslexists) ? &descsetlayoutstemp[scenedslexists ? 0 : 1] : nullptr,
				pushconstantrange.size == 0 ? 0u : 1u,
				&pushconstantrange
		};
		vkCreatePipelineLayout(vulkaninfo.logicaldevice,
							   &pipelinelayoutcreateinfo,
							   nullptr,
							   &pipelineinfo->pipelinelayout);

		VkShaderModule* shadermodule = new VkShaderModule;
		VkPipelineShaderStageCreateInfo* shaderstagecreateinfo = new VkPipelineShaderStageCreateInfo;
		VKSubInitShaders(VK_SHADER_STAGE_COMPUTE_BIT,
						 shaderfilepaths,
						 &shadermodule,
						 &shaderstagecreateinfo,
						 nullptr);
		VkComputePipelineCreateInfo pipelinecreateinfo {
				VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				nullptr,
				0,
				*shaderstagecreateinfo,
				pipelineinfo->pipelinelayout,
				VK_NULL_HANDLE,
				-1
		};
		vkCreateComputePipelines(vulkaninfo.logicaldevice,
								 VK_NULL_HANDLE,
								 1,
								 &pipelinecreateinfo,
								 nullptr,
								 &pipelineinfo->pipeline);
		delete shaderstagecreateinfo;
		vkDestroyShaderModule(vulkaninfo.logicaldevice, *shadermodule, nullptr);
		delete shadermodule;
		return;
	}

	uint32_t numshaderstages = 0;
	for (auto stage: supportedshaderstages) {
		if (stage & shaderstages) numshaderstages++;
	}
	VkShaderModule* shadermodules = new VkShaderModule[numshaderstages];
	VkPipelineShaderStageCreateInfo* shaderstagecreateinfos = new VkPipelineShaderStageCreateInfo[numshaderstages];
	VKSubInitShaders(shaderstages, shaderfilepaths, &shadermodules, &shaderstagecreateinfos, nullptr);

	/* currently, this is redundant with some earlier code. fix this asap. figure out a good elegant system for creating
	 * and storing dsl's. what we had before seemed fine (although adding stuff to a dsl was a little clunky). */

	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice,
								&descsetlayoutcreateinfos[0],
								nullptr,
								&pipelineinfo->scenedsl);
	vkCreateDescriptorSetLayout(vulkaninfo.logicaldevice,
								&descsetlayoutcreateinfos[1],
								nullptr,
								&pipelineinfo->objectdsl);
	VkDescriptorSetLayout descsetlayoutstemp[2] = {pipelineinfo->scenedsl, pipelineinfo->objectdsl};
	bool scenedslexists = descsetlayoutcreateinfos[0].bindingCount != 0u,
			objectdslexists = descsetlayoutcreateinfos[1].bindingCount != 0u;
	VkPipelineLayoutCreateInfo pipelinelayoutcreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			(uint32_t)scenedslexists + (uint32_t)objectdslexists,
			(scenedslexists | objectdslexists) ? &descsetlayoutstemp[scenedslexists ? 0 : 1] : nullptr,
			pushconstantrange.size == 0 ? 0u : 1u,
			&pushconstantrange
	};
	vkCreatePipelineLayout(vulkaninfo.logicaldevice, &pipelinelayoutcreateinfo, nullptr, &pipelineinfo->pipelinelayout);

	VkPipelineInputAssemblyStateCreateInfo inputassemblystatecreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			nullptr,
			0,
			(shaderstages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) ?
			VK_PRIMITIVE_TOPOLOGY_PATCH_LIST :
			((pipelineinfo == &GraphicsHandler::vulkaninfo.linegraphicspipeline) ?
			 VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
			VK_FALSE
	};
	VkPipelineTessellationStateCreateInfo tessstatecreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			nullptr,
			0,
			3
	};
	bool issm = pipelineinfo == &GraphicsHandler::vulkaninfo.shadowmapgraphicspipeline;
	VkViewport viewporttemp {
			0.0f,
			0.0f,
			issm ? 4096 : (float)vulkaninfo.swapchainextent.width,
			issm ? 4096 : (float)vulkaninfo.swapchainextent.height,
			0.0f,               // TODO: try changing this to -1.f!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			1.0f
	};
	VkExtent2D tempext {4096, 4096};
	VkRect2D scissortemp {
			{0, 0},
			issm ? tempext : vulkaninfo.swapchainextent
	};
	VkPipelineViewportStateCreateInfo viewportstatecreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			nullptr,
			0,
			1,
			&viewporttemp,
			1,
			&scissortemp
	};
	VkPipelineRasterizationStateCreateInfo rasterizationstatecreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE,
			VK_FALSE,
			VK_POLYGON_MODE_FILL,
			issm ? VK_CULL_MODE_FRONT_BIT : (pipelineinfo == &GraphicsHandler::vulkaninfo.grassgraphicspipeline
											 ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT),
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VK_FALSE,                       // TODO: try enabling this for shadowmap someday
			0.0f,
			0.0f,
			0.0f,
			1.0f
	};
	VkPipelineMultisampleStateCreateInfo multisamplestatecreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_SAMPLE_COUNT_1_BIT,              // TODO: try messing with this
			VK_FALSE,
			1.0f,
			nullptr,
			VK_FALSE,
			VK_FALSE
	};
	VkPipelineDepthStencilStateCreateInfo depthstencilstatecreateinfo;
	if (depthtest) {
		depthstencilstatecreateinfo = {
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
				0.0f,                   // TODO: try changing this to -1.f
				1.0f
		};
	} else {
		depthstencilstatecreateinfo = {
				VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				nullptr,
				0,
				VK_FALSE,
				VK_FALSE,
				VK_COMPARE_OP_NEVER,
				VK_FALSE,
				VK_FALSE,
				{},
				{},
				0.0f,
				1.0f
		};
	}
	VkPipelineColorBlendAttachmentState colorblendattachmentstate {
			VK_TRUE,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_OP_SUBTRACT,
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo colorblendstatecreateinfo {
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE,
			VK_LOGIC_OP_AND,
			1, &colorblendattachmentstate,
			{0.0f, 0.0f, 0.0f, 0.0f}
	};
	VkGraphicsPipelineCreateInfo pipelinecreateinfo = {
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			nullptr,
			0,
			numshaderstages,
			shaderstagecreateinfos,
			&vertexinputstatecreateinfo,
			&inputassemblystatecreateinfo,
			shaderstages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ? &tessstatecreateinfo : nullptr,
			&viewportstatecreateinfo,
			&rasterizationstatecreateinfo,
			&multisamplestatecreateinfo,
			&depthstencilstatecreateinfo,
			&colorblendstatecreateinfo,
			nullptr,
			pipelineinfo->pipelinelayout,
			issm ? vulkaninfo.templateshadowrenderpass : vulkaninfo.primaryrenderpass,  // TODO: add renderpass arg
			0,
			VK_NULL_HANDLE,
			-1
	};
	vkCreateGraphicsPipelines(vulkaninfo.logicaldevice,
							  VK_NULL_HANDLE,
							  1,
							  &pipelinecreateinfo,
							  nullptr,
							  &pipelineinfo->pipeline);

	delete[] shaderstagecreateinfos;
	for (unsigned char x = 0; x < numshaderstages; x++)
		vkDestroyShaderModule(vulkaninfo.logicaldevice, shadermodules[x], nullptr);
	delete[] shadermodules;
}

void GraphicsHandler::VKSubInitShaders (
		VkShaderStageFlags stages,
		const char** filepaths,
		VkShaderModule** modules,
		VkPipelineShaderStageCreateInfo** createinfos,
		VkSpecializationInfo* specializationinfos) {
	std::ifstream filestream;
	size_t shadersrcsize;
	char* shadersrc;
	unsigned char stagecounter = 0;
	VkShaderModuleCreateInfo modcreateinfo;
	for (unsigned char x = 0; x < NUM_SHADER_STAGES_SUPPORTED; x++) {
		if (stages & supportedshaderstages[x]) {
			filestream = std::ifstream(filepaths[stagecounter], std::ios::ate | std::ios::binary);
			shadersrcsize = filestream.tellg();
			shadersrc = new char[shadersrcsize];
			filestream.seekg(0);
			filestream.read(&shadersrc[0], shadersrcsize);
			filestream.close();
			modcreateinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			modcreateinfo.pNext = nullptr;
			modcreateinfo.flags = 0;
			modcreateinfo.codeSize = shadersrcsize;
			modcreateinfo.pCode = reinterpret_cast<const uint32_t*>(&shadersrc[0]);
			vkCreateShaderModule(vulkaninfo.logicaldevice,
								 &modcreateinfo,
								 nullptr,
								 &(*modules)[stagecounter]);
			(*createinfos)[stagecounter].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			(*createinfos)[stagecounter].pNext = nullptr;
			(*createinfos)[stagecounter].flags = 0;
			(*createinfos)[stagecounter].stage = supportedshaderstages[x];
			(*createinfos)[stagecounter].module = (*modules)[stagecounter];
			(*createinfos)[stagecounter].pName = "main";
			(*createinfos)[stagecounter].pSpecializationInfo = specializationinfos ? &specializationinfos[stagecounter]
																				   : nullptr;
			delete[] shadersrc;
			stagecounter++;
		}
	}
}

void GraphicsHandler::VKSubInitDepthBuffer () {
	VkImageCreateInfo imgcreateinfo {
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

	VkMemoryRequirements memrequirements {};
	vkGetImageMemoryRequirements(vulkaninfo.logicaldevice, vulkaninfo.depthbuffer.image, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops {};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
	uint32_t memindex = -1u;
	for (uint32_t x = 0; x < physicaldevicememprops.memoryTypeCount; x++) {
		if (memrequirements.memoryTypeBits & (1 << x)
			&& ((physicaldevicememprops.memoryTypes[x].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			memindex = x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, &(vulkaninfo.depthbuffer.memory));
	vkBindImageMemory(vulkaninfo.logicaldevice, vulkaninfo.depthbuffer.image, vulkaninfo.depthbuffer.memory, 0);

	VkImageViewCreateInfo imgviewcreateinfo {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			vulkaninfo.depthbuffer.image,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_D32_SFLOAT,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			 VK_COMPONENT_SWIZZLE_IDENTITY},
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
												   1,
												   VK_FORMAT_D32_SFLOAT,
												   VK_IMAGE_LAYOUT_UNDEFINED,
												   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void GraphicsHandler::VKHelperCreateAndAllocateBuffer (
		VkBuffer* buffer,
		VkDeviceSize buffersize,
		VkBufferUsageFlags bufferusage,
		VkDeviceMemory* buffermemory,
		VkMemoryPropertyFlags reqprops) {
	if (*buffer != VK_NULL_HANDLE && buffer != &GraphicsHandler::vulkaninfo.stagingbuffer) {
		vkDestroyBuffer(vulkaninfo.logicaldevice, *buffer, nullptr);
		vkFreeMemory(vulkaninfo.logicaldevice, *buffermemory, nullptr);
	}
	VkBufferCreateInfo buffercreateinfo {
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

	VkMemoryRequirements memrequirements {};
	vkGetBufferMemoryRequirements(vulkaninfo.logicaldevice, *buffer, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops {};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
	uint32_t memindex = -1u;
	for (uint32_t x = 0; x < physicaldevicememprops.memoryTypeCount; x++) {
		if (memrequirements.memoryTypeBits & (1 << x)
			&& ((physicaldevicememprops.memoryTypes[x].propertyFlags & reqprops) == reqprops)) {
			memindex = x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo.logicaldevice, &memallocateinfo, nullptr, buffermemory);
	vkBindBufferMemory(vulkaninfo.logicaldevice, *buffer, *buffermemory, 0);
}

void GraphicsHandler::VKHelperCpyBufferToBuffer (
		VkBuffer* src,
		VkBuffer* dst,
		VkDeviceSize size) {
	VkBufferCopy region {
			0,
			0,
			size
	};
	VkCommandBufferBeginInfo cmdbuffbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	vkBeginCommandBuffer(vulkaninfo.interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdCopyBuffer(vulkaninfo.interimcommandbuffer, *src, *dst, 1, &region);
	vkEndCommandBuffer(vulkaninfo.interimcommandbuffer);
	VkSubmitInfo subinfo {
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

void GraphicsHandler::VKHelperCpyBufferToImage (
		VkBuffer* src,
		VkImage* dst,
		VkFormat format,
		uint32_t numlayers,
		uint32_t rowpitch,
		uint32_t horires, uint32_t vertres) {
	VkBufferImageCopy imgcopy {
			0,
			rowpitch,
			vertres,
			{
					format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					0,
					numlayers
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
	VkCommandBufferBeginInfo cmdbuffbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	vkBeginCommandBuffer(vulkaninfo.interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdCopyBufferToImage(vulkaninfo.interimcommandbuffer, *src, *dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
						   &imgcopy);
	vkEndCommandBuffer(vulkaninfo.interimcommandbuffer);
	VkSubmitInfo subinfo {
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

void GraphicsHandler::VKHelperTransitionImageLayout (
		VkImage image,
		uint32_t numlayers,
		VkFormat format,
		VkImageLayout oldlayout,
		VkImageLayout newlayout) {
	VkCommandBufferBeginInfo cmdbuffbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	VkImageMemoryBarrier imgmembarrier {
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
					format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					1,
					0,
					numlayers
			}
	};
	VkPipelineStageFlags srcmask, dstmask;
	if (oldlayout == VK_IMAGE_LAYOUT_UNDEFINED) {
		imgmembarrier.srcAccessMask = 0;
		srcmask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	} else if (oldlayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imgmembarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcmask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldlayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imgmembarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcmask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldlayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imgmembarrier.srcAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcmask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else if (oldlayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		imgmembarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		srcmask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	if (newlayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imgmembarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstmask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (newlayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imgmembarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstmask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (newlayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		imgmembarrier.dstAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dstmask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else if (newlayout == VK_IMAGE_LAYOUT_GENERAL) {
		imgmembarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		dstmask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	vkBeginCommandBuffer(vulkaninfo.interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdPipelineBarrier(vulkaninfo.interimcommandbuffer, srcmask, dstmask, 0, 0, nullptr, 0, nullptr, 1,
						 &imgmembarrier);
	vkEndCommandBuffer(vulkaninfo.interimcommandbuffer);
	VkSubmitInfo subinfo {
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

void GraphicsHandler::VKHelperInitVertexAndIndexBuffers (
		const std::vector<Vertex>& vertices,
		const std::vector<Tri>& tris,
		VkBuffer* vertexbufferdst,
		VkDeviceMemory* vertexbuffermemorydst,
		VkBuffer* indexbufferdst,
		VkDeviceMemory* indexbuffermemorydst) {
	VKHelperInitVertexBuffer(vertices, vertexbufferdst, vertexbuffermemorydst);

	VkDeviceSize indexbuffersize = sizeof(uint16_t) * 3 * tris.size();
	VKHelperCreateAndAllocateBuffer(&vulkaninfo.stagingbuffer,
									indexbuffersize,
									VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
									&vulkaninfo.stagingbuffermemory,
									VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* indexdatatemp;
	vkMapMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, 0, indexbuffersize, 0, &indexdatatemp);
	uint16_t indextemp[3 * tris.size()];
	for (int x = 0; x < tris.size(); x++) {
		for (int y = 0; y < 3; y++) {
			indextemp[3 * x + y] = tris[x].vertexindices[y];
		}
	}
	memcpy(indexdatatemp, (void*)indextemp, indexbuffersize);
	vkUnmapMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory);

	VKHelperCreateAndAllocateBuffer(indexbufferdst,
									indexbuffersize,
									VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
									indexbuffermemorydst,
									VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VKHelperCpyBufferToBuffer(&vulkaninfo.stagingbuffer,
							  indexbufferdst,
							  indexbuffersize);

	vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffer, nullptr);
}

void GraphicsHandler::VKHelperInitVertexBuffer (
		const std::vector<Vertex>& vertices,
		VkBuffer* vertexbufferdst,
		VkDeviceMemory* vertexbuffermemorydst) {


	VkDeviceSize vertexbuffersize = sizeof(Vertex) * vertices.size();
	VKHelperCreateAndAllocateBuffer(&vulkaninfo.stagingbuffer,
									vertexbuffersize,
									VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
									&vulkaninfo.stagingbuffermemory,
									VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* datatemp;
	vkMapMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, 0, vertexbuffersize, 0, &datatemp);
	memcpy(datatemp, (void*)vertices.data(), vertexbuffersize);
	vkUnmapMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory);

	VKHelperCreateAndAllocateBuffer(vertexbufferdst,
									vertexbuffersize,
									VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
									vertexbuffermemorydst,
									VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VKHelperCpyBufferToBuffer(&vulkaninfo.stagingbuffer,
							  vertexbufferdst,
							  vertexbuffersize);

	vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffer, nullptr);
}

void GraphicsHandler::VKHelperInitImage (
		TextureInfo* imgdst,
		uint32_t horires, uint32_t vertres,
		VkFormat format,
		VkMemoryPropertyFlags memprops,
		VkImageUsageFlags usage,
		VkImageLayout finallayout,
		VkImageViewType imgviewtype) {
	imgdst->resolution = {horires, vertres};
	if (horires == 0) imgdst->resolution.width = imgdst->resolution.height;
	else if (vertres == 0) imgdst->resolution.height = imgdst->resolution.width;
	imgdst->format = format;
	imgdst->memoryprops = memprops;
	bool tilingoptimal = imgviewtype == VK_IMAGE_VIEW_TYPE_CUBE
						 || format == VK_FORMAT_D32_SFLOAT
						 || imgdst->type == TEXTURE_TYPE_SUBPASS;

	VkImageCreateInfo imgcreateinfo {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			VkImageCreateFlags(imgviewtype == VK_IMAGE_VIEW_TYPE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0),
			VK_IMAGE_TYPE_2D,
			imgdst->format,
			{imgdst->resolution.width, imgdst->resolution.height, 1},
			1,
			uint32_t(imgviewtype == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1),
			VK_SAMPLE_COUNT_1_BIT,
			VkImageTiling(tilingoptimal ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR),
//		VK_IMAGE_TILING_LINEAR,
			usage,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(vulkaninfo.logicaldevice, &imgcreateinfo, nullptr, &imgdst->image);

	VkMemoryRequirements memrequirements {};
	vkGetImageMemoryRequirements(vulkaninfo.logicaldevice, imgdst->image, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops {};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo.physicaldevice, &physicaldevicememprops);
	uint32_t memindex = -1u;
	for (uint32_t x = 0; x < physicaldevicememprops.memoryTypeCount; x++) {
		if (memrequirements.memoryTypeBits & (1 << x)
			&& ((physicaldevicememprops.memoryTypes[x].propertyFlags & imgdst->memoryprops) == imgdst->memoryprops)) {
			memindex = x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocinfo {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo.logicaldevice, &memallocinfo, nullptr, &imgdst->memory);
	vkBindImageMemory(vulkaninfo.logicaldevice, imgdst->image, imgdst->memory, 0);

	VKHelperTransitionImageLayout(
			imgdst->image,
			imgdst->type == TEXTURE_TYPE_CUBEMAP ? 6 : 1,
			imgdst->format,
			VK_IMAGE_LAYOUT_UNDEFINED,
			finallayout);
	imgdst->layout = finallayout;

	VkImageViewCreateInfo imgviewcreateinfo {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			imgdst->image,
			imgviewtype,
			imgdst->format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			 VK_COMPONENT_SWIZZLE_IDENTITY},
			{format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
			 uint32_t(imgviewtype == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1)}
	};
	vkCreateImageView(vulkaninfo.logicaldevice, &imgviewcreateinfo, nullptr, &imgdst->imageview);

	imgdst->sampler = VK_NULL_HANDLE;
}

void GraphicsHandler::VKHelperInitTexture (
		TextureInfo* texturedst,
		uint32_t horires, uint32_t vertres,
		VkFormat format,
		VkMemoryPropertyFlags memprops,
		TextureType textype,
		VkImageViewType imgviewtype,
		VkSampler sampler) {
	texturedst->type = textype;
	VkImageUsageFlags usage;
	VkImageLayout layout;
	if (textype == TEXTURE_TYPE_SHADOWMAP) {
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
		// TODO: reevaluate TEXTURE_TYPE_SUBPASS usefulness
	else if (textype == TEXTURE_TYPE_SUBPASS) { // entire subpass textype may be useless eventually
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	} else if (textype == TEXTURE_TYPE_SSRR_BUFFER) {
		usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	} else if (memprops & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
		usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	} else {
		usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VKHelperInitImage(
			texturedst,
			horires,
			vertres,
			format,
			memprops,
			usage,
			layout,
			imgviewtype);

	VkDeviceSize size = texturedst->resolution.width * texturedst->resolution.height * VKHelperGetPixelSize(format) *
						(imgviewtype == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1);
	//below stuff has redundant code in updatewholeimages...is it necessary?
	if (imgviewtype != VK_IMAGE_VIEW_TYPE_CUBE && format != VK_FORMAT_D32_SFLOAT && textype != TEXTURE_TYPE_SUBPASS) {
		VkSubresourceLayout subresourcelayout;
		VkImageSubresource imgsubresource {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
		vkGetImageSubresourceLayout(vulkaninfo.logicaldevice, texturedst->image, &imgsubresource, &subresourcelayout);
		size = subresourcelayout.size;
	}

	if (textype != TEXTURE_TYPE_SHADOWMAP && textype != TEXTURE_TYPE_SUBPASS) {
		char* emptydata = new char[size];
		memset(emptydata, 0, size);
		VKHelperUpdateWholeTexture(texturedst, reinterpret_cast<void*>(emptydata));
		delete[] emptydata;
	}
//	texturedst->layout=textype==TEXTURE_TYPE_SUBPASS?VK_IMAGE_LAYOUT_GENERAL|VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texturedst->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// wait what?? why isn't layout = layout???
	// TODO: come back here when program is functional to fiddle
	texturedst->sampler = sampler;
	texturedst->setUVPosition(glm::vec2(0., 0.));
	texturedst->setUVScale(glm::vec2(1., 1.));
	texturedst->setUVRotation(0.);
}

void GraphicsHandler::VKHelperUpdateWholeTexture (
		TextureInfo* texdst,
		void* src) {
	//having the cubemap ternary exp for layers everywhere feels wrong...maybe polish up later
	//do we have any other uses for layers other than 6 for cubemap, 1 for everything else?
	bool querysubresource = texdst->type != TEXTURE_TYPE_CUBEMAP && texdst->format != VK_FORMAT_D32_SFLOAT &&
							texdst->type != TEXTURE_TYPE_SUBPASS;
	uint32_t pixelsize = VKHelperGetPixelSize(texdst->format);
	VkSubresourceLayout subresourcelayout;
	VkImageSubresource imgsubresource {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
	VkDeviceSize size = texdst->resolution.width * texdst->resolution.height * pixelsize *
						(texdst->type == TEXTURE_TYPE_CUBEMAP ? 6 : 1);
	if (querysubresource) {
		vkGetImageSubresourceLayout(vulkaninfo.logicaldevice, texdst->image, &imgsubresource, &subresourcelayout);
		size = subresourcelayout.size;
	}
	bool devloc = texdst->memoryprops & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (devloc) {
		GraphicsHandler::VKHelperCreateAndAllocateBuffer(
				&(vulkaninfo.stagingbuffer),
				size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				&(vulkaninfo.stagingbuffermemory),
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	void* dst;
	vkMapMemory(
			vulkaninfo.logicaldevice,
			devloc ? vulkaninfo.stagingbuffermemory : texdst->memory,
			0,
			VK_WHOLE_SIZE,
			0,
			&dst);
	VkDeviceSize pitch;
	if (querysubresource) pitch = subresourcelayout.rowPitch;
	else pitch = texdst->resolution.width * pixelsize;
	char* dstscan = reinterpret_cast<char*>(dst), * srcscan = reinterpret_cast<char*>(src);
	for (uint32_t x = 0; x < texdst->resolution.height * (texdst->type == TEXTURE_TYPE_CUBEMAP ? 6 : 1); x++) {
		memcpy(reinterpret_cast<void*>(dstscan), reinterpret_cast<void*>(srcscan),
			   texdst->resolution.width * pixelsize);
		dstscan += pitch;
		srcscan += texdst->resolution.width * pixelsize;
	}
	vkUnmapMemory(vulkaninfo.logicaldevice, devloc ? vulkaninfo.stagingbuffermemory : texdst->memory);
	if (devloc) {
		if (texdst->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			VKHelperTransitionImageLayout(
					texdst->image,
					texdst->type == TEXTURE_TYPE_CUBEMAP ? 6 : 1,
					texdst->format,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}
		VKHelperCpyBufferToImage(       //may have to factor layers into this mess
				&vulkaninfo.stagingbuffer,
				&texdst->image,
				texdst->format,
				texdst->type == TEXTURE_TYPE_CUBEMAP ? 6 : 1,
				pitch / pixelsize,
				texdst->resolution.width, texdst->resolution.height);
		vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, nullptr);
		vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffer, nullptr);
		VKHelperTransitionImageLayout(
				texdst->image,
				texdst->type == TEXTURE_TYPE_CUBEMAP ? 6 : 1,
				texdst->format,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void GraphicsHandler::VKHelperUpdateUniformBuffer (
		uint32_t elementcount,
		VkDeviceSize elementsize,
		VkDeviceMemory buffermemory,
		void* data) {
	VkPhysicalDeviceProperties physdevprops;
	vkGetPhysicalDeviceProperties(vulkaninfo.physicaldevice, &physdevprops);
	VkDeviceSize roundedupelementsize = elementsize;
	if (elementsize % physdevprops.limits.minUniformBufferOffsetAlignment != 0) {
		roundedupelementsize =
				(1 + floor((float)elementsize / (float)physdevprops.limits.minUniformBufferOffsetAlignment))
				* physdevprops.limits.minUniformBufferOffsetAlignment;
	}
	void* datatemp;
	vkMapMemory(vulkaninfo.logicaldevice,
				buffermemory,
				0,
				roundedupelementsize * elementcount,
				0,
				&datatemp);
//	std::cout<<"update unibuf called on unibufmem "<<buffermemory<<" for "<<elementcount<<" elements, each of size "<<
//		elementsize<<" bytes. rounded up to "<<roundedupelementsize<<" bytes for physical device. memory mapped and"
//																	   "being written to "<<datatemp<<std::endl;
//	std::cout<<"byte scan:"<<std::endl;
	char* datadst = (char*)datatemp, * datasrc = (char*)data;
	for (uint32_t x = 0; x < elementcount; x++) {
		memcpy((void*)datadst, (void*)datasrc, elementsize);
//		for(uint32_t x=0;x<elementsize;x++){
//			std::cout<<std::hex<<(int)(unsigned char)(*datasrc)<<' ';
//			if(x%4==3) std::cout<<"| ";
//			datasrc++;
//		}
//		std::cout<<"\n";
		datadst += roundedupelementsize;
		datasrc += elementsize;
	}
	vkUnmapMemory(vulkaninfo.logicaldevice, buffermemory);
}

void GraphicsHandler::VKHelperUpdateStorageBuffer (
		uint32_t elementcount,
		VkDeviceSize elementsize,
		VkBuffer* buffer,
		VkDeviceMemory buffermemory,
		void* data) {
	VkPhysicalDeviceProperties physdevprops;
	vkGetPhysicalDeviceProperties(vulkaninfo.physicaldevice, &physdevprops);
	VkDeviceSize roundedupelementsize = elementsize;
	// TODO: change to minStorageBuffer...
	if (elementsize % physdevprops.limits.minUniformBufferOffsetAlignment != 0) {
		roundedupelementsize =
				(1 + floor((float)elementsize / (float)physdevprops.limits.minUniformBufferOffsetAlignment))
				* physdevprops.limits.minUniformBufferOffsetAlignment;
	}
	VKHelperCreateAndAllocateBuffer(
			&GraphicsHandler::vulkaninfo.stagingbuffer,
			roundedupelementsize * elementcount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			&GraphicsHandler::vulkaninfo.stagingbuffermemory,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* datatemp;
	vkMapMemory(vulkaninfo.logicaldevice,
				GraphicsHandler::vulkaninfo.stagingbuffermemory,
				0,
				roundedupelementsize * elementcount,
				0,
				&datatemp);
	char* datadst = (char*)datatemp, * datasrc = (char*)data;
	for (uint32_t x = 0; x < elementcount; x++) {
		memcpy((void*)datadst, (void*)datasrc, elementsize);
		datadst += roundedupelementsize;
		datasrc += elementsize;
	}
	vkUnmapMemory(vulkaninfo.logicaldevice, GraphicsHandler::vulkaninfo.stagingbuffermemory);
	VKHelperCpyBufferToBuffer(
			&GraphicsHandler::vulkaninfo.stagingbuffer,
			buffer,
			roundedupelementsize * elementcount);
	vkFreeMemory(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo.logicaldevice, vulkaninfo.stagingbuffer, nullptr);
}

void GraphicsHandler::VKHelperRecordImageTransition (
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldlayout,
		VkImageLayout newlayout) {
	VkImageMemoryBarrier imgmembar {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	vkCmdPipelineBarrier(cmdbuffer,
						 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 0,
						 0, nullptr,
						 0, nullptr,
						 1, &imgmembar);
}

glm::vec3 GraphicsHandler::mat4TransformVec3 (glm::mat4 M, glm::vec3 v) {
	glm::vec4 w = M * glm::vec4(v.x, v.y, v.z, 1.);
	w /= w.w;
	return glm::vec3(w.x, w.y, w.z);
}

glm::mat4 GraphicsHandler::projectFromView (glm::mat4 P, glm::vec3 p, glm::vec3 c, glm::vec3 u) {
	// there is likely a more efficient version of this using p, c, and u to `determine the inverse view matrix w/o using mat4TransformVec3
//	return glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));
	glm::vec3 look = p - c, newup = glm::cross(glm::cross(u, look), look);
	PRINT_TRIPLE(newup);
	std::cout << std::endl;
	glm::mat4 V = glm::lookAt(p, c, glm::vec3(0.f, 1.f, 0.f));
	return glm::lookAt(mat4TransformVec3(V, glm::vec3(0.f)),
					   mat4TransformVec3(V, glm::vec3(0.f, 0.f, -1.f)),
					   mat4TransformVec3(V, glm::vec3(0.f, 1.f, 0.f))) * P * V;
}

void GraphicsHandler::makeRectPrism (glm::vec3** dst, glm::vec3 min, glm::vec3 max) {
	(*dst)[0] = max;
	(*dst)[1] = glm::vec3(max.x, max.y, min.z);
	(*dst)[2] = glm::vec3(max.x, min.y, min.z);
	(*dst)[3] = glm::vec3(min.x, max.y, max.z);
	(*dst)[4] = glm::vec3(min.x, min.y, max.z);
	(*dst)[5] = glm::vec3(min.x, max.y, min.z);
	(*dst)[6] = glm::vec3(max.x, min.y, max.z);
	(*dst)[7] = min;
}

inline VkDeviceSize GraphicsHandler::VKHelperGetPixelSize (VkFormat format) {
	switch (format) {
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16u;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 12u;
		case VK_FORMAT_R8G8B8A8_UNORM:
			return 4u;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return 4u;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return 4u;
		case VK_FORMAT_R32_SFLOAT:
			return 4u;
		case VK_FORMAT_D32_SFLOAT:
			return 4u;
		default:
			return -1u;
	}
}

VKAPI_ATTR VkBool32

VKAPI_CALL GraphicsHandler::debugCallback (
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackdata,
		void* userdata) {
	if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return VK_FALSE;
	if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) std::cout << callbackdata->pMessage << std::endl;
	uint32_t newlinecounter = 0u, charcounter = 0u;
	if (callbackdata->messageIdNumber != -1539028524) {
		while (callbackdata->pMessage[charcounter] != '\0') {
			//		troubleshootingsstrm<<callbackdata->pMessage[charcounter];
			if (newlinecounter > 50u) {
				//			troubleshootingsstrm<<'\n';
				newlinecounter = 0u;
			}
			newlinecounter++;
			charcounter++;
		}
		std::cout << callbackdata->pMessage << std::endl;
	}
	return VK_FALSE;
}

VkResult GraphicsHandler::CreateDebugUtilsMessengerEXT (
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* createinfo,
		const VkAllocationCallbacks* allocator,
		VkDebugUtilsMessengerEXT* debugutilsmessenger) {
	auto function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
																			  "vkCreateDebugUtilsMessengerEXT");
	return function(instance, createinfo, allocator, debugutilsmessenger);
}

void GraphicsHandler::DestroyDebugUtilsMessengerEXT (
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugutilsmessenger,
		const VkAllocationCallbacks* allocator) {
	auto function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
																			   "vkDestroyDebugUtilsMessengerEXT");
	function(instance, debugutilsmessenger, allocator);
}

void GraphicsHandler::glfwErrorCallback (int error, const char* description) {
	std::cout << "oopsie, glfw error (how'd you even do that? we do almost nothing with that lib lol...)" << std::endl;
	std::cout << description << std::endl;
}

void GraphicsHandler::keystrokeCallback (GLFWwindow* window, int key, int scancode, int action, int mods) {
	int previous = keyvalues.find(key)->second.currentvalue;
	keyvalues.find(key)->second = {
			action,
			previous,
			action == GLFW_RELEASE && previous == GLFW_PRESS,
			action == GLFW_PRESS && previous == GLFW_RELEASE
	};
}