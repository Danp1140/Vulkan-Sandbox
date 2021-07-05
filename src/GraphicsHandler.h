//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

#include <fstream>
#include <sstream>

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

typedef enum PipelineID{
	PRIMARY_GRAPHICS_PIPELINE
}PipelineID;

typedef struct VulkanInfo{
	GLFWwindow*window;
	int horizontalres, verticalres, width, height;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicaldevice;
	VkDevice logicaldevice;
	//better way to organize these?
	VkQueue graphicsqueue, presentationqueue;
	//perhaps should break up the bottom into however many uint32_ts, instead of one pointer/array
	uint32_t*queuefamilyindices;
	VkSwapchainKHR swapchain;
	uint32_t numswapchainimages;
	VkImage*swapchainimages;
	VkImageView*swapchainimageviews;
	VkExtent2D swapchainextent;
	VkRenderPass primaryrenderpass;
	std::map<PipelineID, VkPipeline> pipelines;
	VkFramebuffer*framebuffers;
	VkCommandPool commandpool;
	VkCommandBuffer*commandbuffers;
	VkSemaphore imageavailablesemaphore, renderfinishedsemaphore;
} VulkanInfo;

class GraphicsHandler{
private:
	static VulkanInfo vulkaninfo;
//  extent is redundant with hori/vertres variables
	static VkExtent2D swapextent;
	static VkPipelineShaderStageCreateInfo shaderstagecreateinfos[2];
	static VkPipelineLayout pipelinelayout;
	static VkPipeline primarygraphicspipeline;

	Camera*primarycamera;
	std::vector<Light*>lights;
	Light**lastlightsptr;
	std::vector<Mesh*>dynamicshadowcastingmeshes, landscapemeshes;
	std::vector<const Mesh*> staticshadowcastingmeshes;
	Mesh**laststaticscmptr;
	Grass*testgrass;
	Text*troubleshootingtext;
	double lasttime;

//	static void GLInit(bool printstuff);
	static void VKInit();
	static void VKSubInitWindow();
	static void VKSubInitInstance();
	static void VKSubInitDevices();
	static void VKSubInitQueues();
	static void VKSubInitSwapchain();
	static void VKSubInitRenderpass();
	static void VKSubInitGraphicsPipeline();
	static void VKSubInitFramebuffers();
	static void VKSubInitCommandPool();
	static void VKSubInitSemaphores();
public:
	GraphicsHandler();
	~GraphicsHandler();
	void draw();
	void drawShittyTextureMonitor();
	void sendLightUniforms();
	bool shouldClose();
	static GLuint linkShaders(char shadertypes, ...);
	static void createGraphicsPipelineSPIRV(const char*vertshaderfilepath, const char*fragshaderfilepath);
};


#endif //SANDBOX_VIEWPORT_H
