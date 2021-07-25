//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

//some of these includes may be redundant
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <map>
#include <iostream>
#include <random>
#include <cstdarg>

#include <png/png.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan/vk_enum_string_helper.h>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>

#define GLFW_INCLUDE_VULKAN
#define PRINT_TRIPLE(vec3) std::cout<<'('<<vec3[0]<<", "<<vec3[1]<<", "<<vec3[2]<<")\n"
#define PRINT_QUAD(vec4) std::cout<<'('<<vec4[0]<<", "<<vec4[1]<<", "<<vec4[2]<<", "<<vec4[3]<<")"
#define MAX_FRAMES_IN_FLIGHT 2
#define VULKAN_DEBUG 1
#define VULKAN_SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#define VULKAN_MAX_LIGHTS 8

typedef enum ChangeFlagBits{
	NO_CHANGE_FLAG_BIT=    0x00000000,
	CAMERA_CHANGE_FLAG_BIT=0x00000001,
	LIGHT_CHANGE_FLAG_BIT= 0x00000002,
	MESH_CHANGE_FLAG_BIT=  0x00000004,
	TEXT_CHANGE_FLAG_BIT=  0x00000008
}ChangeFlagBits;
typedef int ChangeFlag;

typedef struct PipelineInfo{        //some improvmenets could be added/subtracted to this struct
	VkDescriptorSetLayout scenedescriptorsetlayout, meshdescriptorsetlayout;
	VkDescriptorSet*descriptorset;
	VkShaderModule vertexshadermodule, fragmentshadermodule;
	VkPipelineLayout pipelinelayout;
	VkPipeline pipeline;
} PipelineInfo;

typedef struct TextureInfo{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
	VkSampler sampler;
	VkDescriptorImageInfo getDescriptorImageInfo()const{ //consider making function inline
		return {sampler, imageview, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	}
}TextureInfo;

typedef struct PrimaryGraphicsPushConstants{
	glm::mat4 cameravpmatrices, modelmatrix;
	uint32_t numlights;
}PrimaryGraphicsPushConstants;

typedef struct TextPushConstants{
	glm::vec2 position, scale;
	float rotation;
}TextPushConstants;

typedef struct LightUniformBuffer{
	glm::vec3 position;
	float intensity;
	glm::vec3 forward;
	uint32_t lighttype;
	glm::vec4 color;
}LightUniformBuffer;

typedef struct VulkanInfo{
	GLFWwindow*window;
	int horizontalres, verticalres, width, height;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugmessenger;
	VkPhysicalDevice physicaldevice;
	VkDevice logicaldevice;
	VkQueue graphicsqueue, presentationqueue;
	uint32_t graphicsqueuefamilyindex, presentqueuefamilyindex, transferqueuefamilyindex;
	VkSwapchainKHR swapchain;
	uint32_t numswapchainimages;
	VkImage*swapchainimages;
	VkImageView*swapchainimageviews;
	VkExtent2D swapchainextent;     //  extent is redundant with hori/vertres variables
	VkRenderPass primaryrenderpass;
	PipelineInfo primarygraphicspipeline, textgraphicspipeline;
	VkFramebuffer*framebuffers;
	VkClearValue clears[2];
	VkCommandPool commandpool;
	VkCommandBuffer*commandbuffers, interimcommandbuffer;
	VkSemaphore imageavailablesemaphores[MAX_FRAMES_IN_FLIGHT], renderfinishedsemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence frameinflightfences[MAX_FRAMES_IN_FLIGHT], imageinflightfences[MAX_FRAMES_IN_FLIGHT];
	int currentframeinflight;
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingbuffermemory;
	VkDescriptorPool descriptorpool;
	VkDescriptorSetLayout textdescriptorsetlayout;
	TextureInfo depthbuffer;
	VkBuffer*lightuniformbuffers;
	VkDeviceMemory*lightuniformbuffermemories;
	TextureInfo skybox;
	PrimaryGraphicsPushConstants primarygraphicspushconstants;
} VulkanInfo;

typedef struct Vertex{
	glm::vec3 position, normal;
	glm::vec2 uv;
	inline bool operator==(const Vertex&rhs){
		return (this->position==rhs.position&&this->normal==rhs.normal&&this->uv==rhs.uv);
	}
}Vertex;

typedef struct Tri{     //could also store an average vertex position
	uint16_t vertices[3];
	std::vector<Tri*> adjacencies;
	glm::vec3 algebraicnormal;
}Tri;

class GraphicsHandler{
private:
	static void VKSubInitWindow();
	static void VKSubInitInstance();
	static void VKSubInitDebug();
	static void VKSubInitDevices();
	static void VKSubInitQueues();
	static void VKSubInitSwapchain();
	static void VKSubInitRenderpass();
	static void VKSubInitDescriptorPool(uint32_t nummeshes);
	static void VKSubInitUniformBuffer(PipelineInfo*pipelineinfo,       //perhaps have option for buffer update rate (for minor optimization)
									   uint32_t binding,
									   VkDeviceSize elementsize,
									   uint32_t elementcount,
									   VkBuffer*buffers,
									   VkDeviceMemory*memories);
	static void VKSubInitGraphicsPipeline();
	static void VKSubInitTextGraphicsPipeline();
	static void VKSubInitFramebuffers();
	static void VKSubInitCommandPool();
	static void VKSubInitSemaphoresAndFences();
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
	static void VKSubInitDepthBuffer();
public:
	static VulkanInfo vulkaninfo;
	static ChangeFlag*changeflags;

	GraphicsHandler();
	~GraphicsHandler();
	static void VKInit();
	static void VKInitPipelines(uint32_t nummeshes, uint32_t numlights);
	static VulkanInfo*getVulkanInfoPtr(){return &vulkaninfo;}
	static void VKHelperCreateAndAllocateBuffer(VkBuffer*buffer,
											    VkDeviceSize buffersize,
											    VkBufferUsageFlags bufferusage,
											    VkDeviceMemory*buffermemory,
											    VkMemoryPropertyFlags reqprops);
	static void VKHelperCpyBufferToBuffer(VkBuffer*src,
								          VkBuffer*dst,
								          VkDeviceSize size);
	static void VKHelperCpyBufferToImage(VkBuffer*src,
									     VkImage*dst,
									     uint32_t rowpitch,
									     uint32_t horires,
									     uint32_t vertres);
	static void VKHelperTransitionImageLayout(VkImage image,
										      VkFormat format,
										      VkImageLayout oldlayout,
										      VkImageLayout newlayout);
	static void VKHelperInitTexture(TextureInfo*texturedst,
			                        uint32_t horires,
			                        uint32_t vertres,
			                        void*data,
			                        uint32_t pixelsize,
			                        VkFormat format);
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
