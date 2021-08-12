//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

#pragma clang diagnostic ignored "-Wdeprecated-volatile"

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
#define PRINT_DOUBLE(vec2) std::cout<<'('<<vec2[0]<<", "<<vec2[1]<<")\n"
#define PRINT_TRIPLE(vec3) std::cout<<'('<<vec3[0]<<", "<<vec3[1]<<", "<<vec3[2]<<")\n"
#define PRINT_QUAD(vec4) std::cout<<'('<<vec4[0]<<", "<<vec4[1]<<", "<<vec4[2]<<", "<<vec4[3]<<")"
#define SHIFT_TRIPLE(vec3) '('<<vec3[0]<<", "<<vec3[1]<<", "<<vec3[2]<<")"
#define MAX_FRAMES_IN_FLIGHT 6
#define VULKAN_SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#define VULKAN_MAX_LIGHTS 8
#define VULKAN_NUM_SHADER_STAGES_SUPPORTED 4
const VkShaderStageFlagBits supportedshaderstages[VULKAN_NUM_SHADER_STAGES_SUPPORTED]={
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT};

typedef enum ChangeFlagBits{
	NO_CHANGE_FLAG_BIT                =0x00000000,
	CAMERA_LOOK_CHANGE_FLAG_BIT       =0x00000001,
	CAMERA_POSITION_CHANGE_FLAG_BIT   =0x00000002,
	LIGHT_CHANGE_FLAG_BIT             =0x00000004,
	MESH_CHANGE_FLAG_BIT              =0x00000008,
	TEXT_CHANGE_FLAG_BIT              =0x00000010
}ChangeFlagBits;
typedef int ChangeFlag;
typedef enum TextureTypes{
	TEXTURE_TYPE_STATIC_LOAD,
	TEXTURE_TYPE_STATIC_GENERATE,
	TEXTURE_TYPE_DYNAMIC_GENERATE
}TextureTypes;
typedef int TextureType;

typedef struct KeyInfo{
	int currentvalue, lastvalue;
	bool up, down;
}KeyInfo;

//typedef struct PipelineInfo{        //some improvmenets could be added/subtracted to this struct
//	VkDescriptorSetLayout scenedescriptorsetlayout, meshdescriptorsetlayout;
//	VkDescriptorSet*descriptorset;
//	VkShaderModule vertexshadermodule, fragmentshadermodule;
//	VkPipelineLayout pipelinelayout;
//	VkPipeline pipeline;
//} PipelineInfo;

//still gotta work out these destructors
typedef struct PipelineInfo{
	VkDescriptorSetLayout scenedsl, objectdsl;
	VkPipelineLayout pipelinelayout;
	VkPipeline pipeline;
}PipelineInfo;

//typedef struct DescriptorSetLayoutsInfo{
//	uint32_t numciss, numubos;
//}DescriptorSetLayoutsInfo;

typedef struct TextureInfo{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
	VkSampler sampler;
	void*data;
	TextureType type;
	VkDescriptorImageInfo getDescriptorImageInfo()const{ //consider making function inline
		return {sampler, imageview, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	}
}TextureInfo;

typedef struct PrimaryGraphicsPushConstants{
	glm::mat4 cameravpmatrices, modelmatrix;
	glm::vec2 standinguv;
	uint32_t numlights;
}PrimaryGraphicsPushConstants;

typedef struct TextPushConstants{
	glm::vec2 position, scale;
	float rotation;
}TextPushConstants;

typedef struct SkyboxPushConstants{
	alignas(16) glm::vec3 forward, dx, dy, sunposition;
}SkyboxPushConstants;

typedef struct OceanPushConstants{
	glm::mat4 cameravpmatrices;
}OceanPushConstants;

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
	PipelineInfo primarygraphicspipeline, textgraphicspipeline, skyboxgraphicspipeline, oceangraphicspipeline;
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
	//perhaps move push constants to wangling engine???
	//figure a better way to handle these tbh
	PrimaryGraphicsPushConstants primarygraphicspushconstants;
	SkyboxPushConstants skyboxpushconstants;
	OceanPushConstants oceanpushconstants;
} VulkanInfo;

typedef struct Vertex{
	glm::vec3 position, normal;
	glm::vec2 uv;
	inline bool operator==(const Vertex&rhs){
		return (this->position==rhs.position&&this->normal==rhs.normal&&this->uv==rhs.uv);
	}
}Vertex;

typedef struct Tri{     //could also store an average vertex position
	uint16_t vertexindices[3];
	Vertex*vertices[3];
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
	static void VKSubInitUniformBuffer(VkDescriptorSet**descsets,       //perhaps have option for buffer update rate (for minor optimization)
									   uint32_t binding,
									   VkDeviceSize elementsize,
									   uint32_t elementcount,
									   VkBuffer**buffers,
									   VkDeviceMemory**memories);
//	static void VKSubInitGraphicsPipeline();
//	static void VKSubInitTextGraphicsPipeline();
//	static void VKSubInitSkyboxGraphicsPipeline();
//	static void VKSubInitOceanGraphicsPipeline();
	static void VKSubInitFramebuffers();
	static void VKSubInitCommandPool();
	static void VKSubInitSemaphoresAndFences();
	static void VKSubInitPipeline(PipelineInfo*pipelineinfo,
								  VkShaderStageFlags shaderstages,
								  const char**shaderfilepaths,
								  VkDescriptorSetLayoutCreateInfo*descsetlayoutcreateinfos,
								  VkPushConstantRange pushconstantrange,
						          VkPipelineVertexInputStateCreateInfo vertexinputstatecreateinfo);
	static void VKSubInitLoadShaders(const char*vertexshaderfilepath,
								     const char*fragmentshaderfilepath,
								     VkShaderModule*vertexshadermodule,
								     VkShaderModule*fragmentshadermodule,
								     VkPipelineShaderStageCreateInfo*vertexshaderstatecreateinfo,
								     VkPipelineShaderStageCreateInfo*fragmentshaderstatecreateinfo);
	//have yet to implement this function in all places
	//note: filepaths will be determined in pipeline order
	//(vertex, tess ctrl, tess eval, geometry, fragment)
	static void VKSubInitShaders(VkShaderStageFlags stages,
							     const char**filepaths,
							     VkShaderModule**modules,
							     VkPipelineShaderStageCreateInfo**createinfos,
							     VkSpecializationInfo*specializationinfos);
	static void VKSubInitDepthBuffer();
public:
	static VulkanInfo vulkaninfo;
	static ChangeFlag*changeflags;
	static std::stringstream troubleshootingsstrm;
	static std::map<int, KeyInfo>keyvalues;
	static uint32_t swapchainimageindex;
	//maybe move into pipeline info???
	static VkDescriptorSet*scenedescriptorsets;

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
	static void VKHelperInitTexture(TextureInfo*texturedst,         //heyyyy, why can't we just store memory indices of certain reqs so we dont have to keep looking them up???
			                        uint32_t horires,
			                        uint32_t vertres,
			                        void*data,
			                        VkFormat format,
			                        bool overwrite);       	//format and element size are kinda redundant
	static void VKHelperUpdateTexture(TextureInfo*texturedst,
							          uint32_t miplevel,
							          VkExtent2D offset,
							          VkExtent2D range,
							          void*src,
							          VkFormat format);
	static void VKHelperUpdateUniformBuffer(uint32_t elementcount,
										    VkDeviceSize elementsize,
										    VkDeviceMemory buffermemory,
										    void*data);
	static uint32_t VKHelperGetPixelSize(VkFormat format);
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

	static void keystrokeCallback(GLFWwindow*window, int key, int scancode, int action, int mods);
};


#endif //SANDBOX_VIEWPORT_H
