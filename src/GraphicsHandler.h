//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

#include <fstream>
#include <sstream>
#include <chrono>

#include <png/png.h>

#include "Camera.h"
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

class GraphicsHandler{
private:
	static VulkanInfo vulkaninfo;
//  extent is redundant with hori/vertres variables
	static VkExtent2D swapextent;
	PrimaryGraphicsPushConstants primarygraphicspushconstants;

	Camera*primarycamera;
	std::vector<Light*>lights;
	Light**lastlightsptr;
	std::vector<Mesh*>dynamicshadowcastingmeshes, landscapemeshes;
	std::vector<const Mesh*> staticshadowcastingmeshes;
	Mesh**laststaticscmptr;
	Grass*testgrass;
	Text*troubleshootingtext;
	std::chrono::time_point<std::chrono::high_resolution_clock> lasttime;

//	static void GLInit(bool printstuff);
	static void VKInit();
	static void VKSubInitWindow();
	static void VKSubInitInstance();
	static void VKSubInitDevices();
	static void VKSubInitQueues();
	static void VKSubInitSwapchain();
	static void VKSubInitRenderpass();
	static void VKSubInitGraphicsPipeline(const char*vertexshaderfilepath, const char*fragmentshaderfilepath);
	static void VKSubInitFramebuffers();
	static void VKSubInitCommandPool();
	static void VKSubInitSemaphores();
//	static void setUpStagingBuffer(VkDeviceSize buffersize);
	void recordCommandBuffers();
public:
	GraphicsHandler();
	~GraphicsHandler();
	void draw();
	void drawShittyTextureMonitor();
	void sendLightUniforms();
	bool shouldClose();
	static GLuint linkShaders(char shadertypes, ...);
	static void createGraphicsPipelineSPIRV(const char*vertshaderfilepath, const char*fragshaderfilepath, VkPipelineLayout*pipelinelayout, VkPipelineShaderStageCreateInfo*vertexshaderstagecreateinfo, VkPipelineShaderStageCreateInfo*fragmentshaderstagecreateinfo);
};


#endif //SANDBOX_VIEWPORT_H
