//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

#include <fstream>
#include <sstream>
#include <chrono>

#include <png/png.h>

#include "PhysicsHandler.h"
#include "Light.h"
#include "Grass.h"
#include "Text.h"

#define VULKAN_DEBUG 1
#define VULKAN_SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
//#define VULKAN_SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_R8G8B8A8_SRGB
#define VULKAN_SWAPCHAIN_COLOR_SPACE VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
#define VULKAN_SWAPCHAIN_PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
//actually the two places i use this value i *think* can be different values
#define VULKAN_NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

typedef struct PrimaryGraphicsPushConstants{
	glm::mat4 cameravpmatrices, modelmatrix;
}PrimaryGraphicsPushConstants;

typedef struct LightUniformBuffer{
	glm::vec3 position;
	float intensity;
	glm::vec3 forward;
	uint32_t lighttype;
	glm::vec4 color;
}LightUniformBuffer;

class GraphicsHandler{
private:
	static VulkanInfo vulkaninfo;
	PrimaryGraphicsPushConstants primarygraphicspushconstants;

	Camera*primarycamera;
	PhysicsHandler physicshandler;
	std::vector<Light*>lights;
	Light**lastlightsptr;
	std::vector<Mesh*>dynamicshadowcastingmeshes, landscapemeshes;
	std::vector<const Mesh*> staticshadowcastingmeshes;
	Mesh**laststaticscmptr;
	Grass*testgrass;
	Text*troubleshootingtext;
	std::chrono::time_point<std::chrono::high_resolution_clock> lasttime;

	static void VKInit();
	static void VKSubInitWindow();
	static void VKSubInitInstance();
	static void VKSubInitDebug();
	static void VKSubInitDevices();
	static void VKSubInitQueues();
	static void VKSubInitSwapchain();
	static void VKSubInitRenderpass();
	static void VKSubInitDescriptorPool(uint32_t nummeshes);
	//perhaps have option for buffer update rate (for minor optimization)
	static void VKSubInitUniformBuffer(PipelineInfo*pipelineinfo,
									   uint32_t binding,
									   VkDeviceSize buffersize,
									   VkBuffer*buffers,
									   VkDeviceMemory*memories);
	static void VKSubInitGraphicsPipeline();
	static void VKSubInitTextGraphicsPipeline();
	static void VKSubInitFramebuffers();
	static void VKSubInitCommandPool();
	static void VKSubInitSemaphoresAndFences();
	void recordCommandBuffer(uint32_t index);
	static void VKSubInitPipeline(PipelineInfo*pipelineinfo,
								  uint32_t numdescriptorsets,
								  VkDescriptorSetLayout*descriptorsetlayouts,
								  VkPushConstantRange*pushconstantrange,
								  VkRenderPass*renderpass,
						          VkPipelineShaderStageCreateInfo*shadermodules,
						          VkPipelineVertexInputStateCreateInfo*vertexinputstatecreateinfo);
	static void VKSubInitLoadShaders(const char*vertexshaderfilepath,
								     const char*fragmentshaderfilepath,
								     VkShaderModule*vertexshadermodule,
								     VkShaderModule*fragmentshadermodule,
								     VkPipelineShaderStageCreateInfo*vertexshaderstatecreateinfo,
								     VkPipelineShaderStageCreateInfo*fragmentshaderstatecreateinfo);
public:
	GraphicsHandler();
	~GraphicsHandler();
	void draw();
	bool shouldClose();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
													    VkDebugUtilsMessageTypeFlagsEXT type,
													    const VkDebugUtilsMessengerCallbackDataEXT*callbackdata,
													    void*userdata);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
											     const VkDebugUtilsMessengerCreateInfoEXT*createinfo,
											     const VkAllocationCallbacks*allocator,
											     VkDebugUtilsMessengerEXT*debugutilsmessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                  VkDebugUtilsMessengerEXT debugutilsmessenger,
                                                  const VkAllocationCallbacks*allocator);
};


#endif //SANDBOX_VIEWPORT_H
