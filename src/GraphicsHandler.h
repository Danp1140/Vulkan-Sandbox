//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

#pragma clang diagnostic ignored "-Wdeprecated-volatile"

#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <map>
#include <iostream>
#include <random>
#include <cstdarg>

#define VK_ENABLE_BETA_EXTENSIONS

#include <png/png.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan/vk_enum_string_helper.h>
//#include <vulkan/vulkan/vulkan_macos.h>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>

#define GLFW_INCLUDE_VULKAN

#define ANSI_BLACK_FORE "\033[30m"
#define ANSI_RED_FORE "\033[31m"
#define ANSI_GREEN_FORE "\033[32m"
#define ANSI_BLUE_FORE "\033[34;1m"
#define ANSI_WHITE_FORE "\033[37;97m"
#define ANSI_RESET_FORE "\033[37m"

#define PRINT_DOUBLE(vec2) std::cout<<'('<<vec2[0]<<", "<<vec2[1]<<")\n"
#define PRINT_TRIPLE(vec3) std::cout<<'('<<vec3[0]<<", "<<vec3[1]<<", "<<vec3[2]<<")\n"
#define PRINT_QUAD(vec4) std::cout<<'('<<vec4[0]<<", "<<vec4[1]<<", "<<vec4[2]<<", "<<vec4[3]<<")\n"
#define SHIFT_DOUBLE(vec2) '('<<vec2[0]<<", "<<vec2[1]<<')'
#define SHIFT_TRIPLE(vec3) '('<<vec3[0]<<", "<<vec3[1]<<", "<<vec3[2]<<')'
#define SHIFT_QUAD(vec4) '('<<vec4[0]<<", "<<vec4[1]<<", "<<vec4[2]<<", "<<vec4[3]<<')'

#define MAX_FRAMES_IN_FLIGHT 6
#define VULKAN_SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#define VULKAN_MAX_LIGHTS 2
#define VULKAN_NUM_SHADER_STAGES_SUPPORTED 5
const VkShaderStageFlagBits supportedshaderstages[VULKAN_NUM_SHADER_STAGES_SUPPORTED]={
		VK_SHADER_STAGE_COMPUTE_BIT,
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

typedef enum TextureType{
	TEXTURE_TYPE_DIFFUSE,
	TEXTURE_TYPE_NORMAL,
	TEXTURE_TYPE_HEIGHT,
	TEXTURE_TYPE_CUBEMAP,
	TEXTURE_TYPE_SHADOWMAP,
	TEXTURE_TYPE_MAX
}TextureType;

typedef struct KeyInfo{
	int currentvalue, lastvalue;
	bool up, down;
}KeyInfo;

typedef struct PipelineInfo{
	VkDescriptorSetLayout scenedsl, objectdsl;
	VkPipelineLayout pipelinelayout;
	VkPipeline pipeline;
}PipelineInfo;

typedef struct TextureInfo{
private:
	glm::vec2 scale, position;
	float rotation;
	void updateUVMatrix(){
		uvmatrix=glm::mat4(
				 scale.x*cos(rotation), scale.y*sin(rotation), position.x, 0,
				-scale.x*sin(rotation), scale.y*cos(rotation), position.y, 0,
				         0            ,        0             ,     1     , 0,
				         0            ,        0             ,     0     , 0);
	}
public:
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
	VkSampler sampler;
	VkImageLayout layout;
	VkFormat format;
	VkExtent2D resolution;
	VkMemoryPropertyFlags memoryprops;
	TextureType type;
	glm::mat3 uvmatrix;
	VkDescriptorImageInfo getDescriptorImageInfo()const{ //consider making function inline???
		return {sampler, imageview, layout};
	}
	void setUVScale(glm::vec2 s){
		scale=s;
		updateUVMatrix();
	}
	void setUVRotation(float r){
		rotation=r;
		updateUVMatrix();
	}
	void setUVPosition(glm::vec2 p){
		position=p;
		updateUVMatrix();
	}
}TextureInfo;

typedef struct PrimaryGraphicsPushConstants{
	glm::mat4 cameravpmatrices;
	alignas(16) glm::vec3 camerapos;
	alignas(16) glm::vec2 standinguv;
	uint32_t numlights;
}PrimaryGraphicsPushConstants;

typedef struct TextPushConstants{
	glm::vec2 position, scale;
	float rotation;
}TextPushConstants;

typedef struct SkyboxPushConstants{
	glm::mat4 cameravpmatrices;     //technically may be able to represent data simply via projection matrix and forward vector, but that's an optimization for later
	alignas(16) glm::vec3 sunposition;
}SkyboxPushConstants;

typedef struct OceanPushConstants{
	glm::mat4 cameravpmatrices;
	alignas(16) glm::vec3 cameraposition;
	uint32_t numlights;
}OceanPushConstants;

typedef struct ShadowmapPushConstants{
	glm::mat4 lightvpmatrices, lspmatrix;
}ShadowmapPushConstants;

typedef struct LightUniformBuffer{
	glm::mat4 vpmatrix, lspmatrix;
	glm::vec3 position;
	float intensity;
	glm::vec3 forward;
	uint32_t lighttype;
	glm::vec4 color;
}LightUniformBuffer;

typedef struct MeshUniformBuffer{
	glm::mat4 modelmatrix,
		diffuseuvmatrix,
		normaluvmatrix,
		heightuvmatrix;
}MeshUniformBuffer;

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
	VkRenderPass primaryrenderpass, templateshadowrenderpass, waterrenderpass;
	PipelineInfo primarygraphicspipeline,
				 textgraphicspipeline,
				 skyboxgraphicspipeline,
				 oceangraphicspipeline,
				 oceancomputepipeline,
				 grassgraphicspipeline,
				 shadowmapgraphicspipeline,
				 texmongraphicspipeline;
	VkFramebuffer*primaryframebuffers, *waterframebuffers;
	VkClearValue clears[2];
	VkCommandPool commandpool;
	VkCommandBuffer*commandbuffers, interimcommandbuffer;
	VkSemaphore imageavailablesemaphores[MAX_FRAMES_IN_FLIGHT], renderfinishedsemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence*submitfinishedfences, *recordinginvalidfences, *presentfinishedfences;
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
	glm::mat4 grasspushconstants;
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

const VkCommandBufferBeginInfo cmdbufferbegininfo{
	VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	nullptr,
	0,
	nullptr
};

//ig we only really need those for that one method, we can probably put them in there...
const VkDescriptorSetLayoutBinding scenedslbindings[2]{{
	     0,
	     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	     VULKAN_MAX_LIGHTS,
	     VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	     nullptr
	 }, {
		 1,
	     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	     1,
	     VK_SHADER_STAGE_FRAGMENT_BIT,
	     nullptr}};

const VkDescriptorSetLayoutCreateInfo scenedslcreateinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		2,
		&scenedslbindings[0]
};

class GraphicsHandler{
private:
	static void VKSubInitWindow();
	static void VKSubInitInstance();
	static void VKSubInitDebug();
	static void VKSubInitDevices();
	static void VKSubInitQueues();
	static void VKSubInitSwapchain();
	static void VKSubInitRenderpasses();
	static void VKSubInitDescriptorPool(uint32_t nummeshes);
	static void VKSubInitFramebuffers();
	static void VKSubInitCommandPool();
	static void VKSubInitSemaphoresAndFences();
	static void VKSubInitPipeline(
			PipelineInfo*pipelineinfo,
			VkShaderStageFlags shaderstages,
			const char**shaderfilepaths,
			VkDescriptorSetLayoutCreateInfo*descsetlayoutcreateinfos,
			VkPushConstantRange pushconstantrange,
			VkPipelineVertexInputStateCreateInfo vertexinputstatecreateinfo,
			bool depthtest);
	static void VKSubInitShaders(
			VkShaderStageFlags stages,
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
	//default/generic sampler props:
		//min/mag filters are nearest
		//repeat at all edges
		//white border
	static VkSampler genericsampler, linearminsampler, linearminmagsampler, clamptobordersampler, depthsampler;

	GraphicsHandler();
	~GraphicsHandler();
	static void VKInit(uint32_t nummeshes);
	static void VKInitPipelines();
	static void VKInitShadowsFramebuffer(VkImageView*shadowmaps);
	static VulkanInfo*getVulkanInfoPtr(){return &vulkaninfo;}
	static void VKSubInitUniformBuffer(
			VkDescriptorSet**descsets,
			uint32_t binding,
			VkDeviceSize elementsize,   //should p make a helper instead of subinit
			uint32_t elementcount,
			VkBuffer**buffers,
			VkDeviceMemory**memories,
			VkDescriptorSetLayout descsetlayout);
	static void VKSubInitStorageBuffer(
			VkDescriptorSet**descriptorsets,
			uint32_t binding,
			VkDeviceSize elementsize,
			uint32_t elementcount,
			VkBuffer**buffers,
			VkDeviceMemory**memories);
	static void VKHelperCreateAndAllocateBuffer(
			VkBuffer*buffer,
			VkDeviceSize buffersize,
			VkBufferUsageFlags bufferusage,
			VkDeviceMemory*buffermemory,
			VkMemoryPropertyFlags reqprops);
	static void VKHelperCpyBufferToBuffer(
			VkBuffer*src,
			VkBuffer*dst,
			VkDeviceSize size);
	static void VKHelperCpyBufferToImage(
			VkBuffer*src,
			VkImage*dst,
			VkFormat format,
			uint32_t numlayers,
			uint32_t rowpitch,
			uint32_t horires, uint32_t vertres);
	static void VKHelperTransitionImageLayout(
			VkImage image,
			uint32_t numlayers,
			VkFormat format,
			VkImageLayout oldlayout,
			VkImageLayout newlayout);
	static void VKHelperInitImage(
			TextureInfo*imgdst,
			uint32_t horires, uint32_t vertres,
			VkFormat format,
			VkMemoryPropertyFlags memprops,
			VkImageUsageFlags usage,
			VkImageLayout finallayout,
			VkImageViewType imgviewtype);
	static void VKHelperInitTexture(
			TextureInfo*texturedst,
			uint32_t horires, uint32_t vertres,
			VkFormat format,
			VkMemoryPropertyFlags memprops,
			TextureType textype,
			VkImageViewType imgviewtype,
			VkSampler sampler);
	static void VKHelperUpdateWholeVisibleTexture(      //because so much information is embedded in TextureInfo, we can consolidate these and all other UpdateWholeTexture helpers (e.g. for cubemaps) (some of the code will be redundant anyways lol)
			TextureInfo*texdst,
			void*src);
	static void VKHelperUpdateWholeTexture(
			TextureInfo*texdst,
			void*src);
	static void VKHelperUpdateUniformBuffer(
			uint32_t elementcount,
			VkDeviceSize elementsize,
			VkDeviceMemory buffermemory,
			void*data);
	static void VKHelperUpdateStorageBuffer(
			uint32_t elementcount,
			VkDeviceSize elementsize,
			VkBuffer*buffer,
			VkDeviceMemory buffermemory,
			void*data);
	static glm::vec3 mat4TransformVec3(glm::mat4 M, glm::vec3 v);
	static void makeRectPrism(glm::vec3**dst, glm::vec3 min, glm::vec3 max);
	static VkDeviceSize VKHelperGetPixelSize(VkFormat format);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT*callbackdata,
			void*userdata);
	static VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT*createinfo,
			const VkAllocationCallbacks*allocator,
			VkDebugUtilsMessengerEXT*debugutilsmessenger);
	static void DestroyDebugUtilsMessengerEXT(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugutilsmessenger,
			const VkAllocationCallbacks*allocator);
	static void glfwErrorCallback(int error, const char*description);

	static void keystrokeCallback(GLFWwindow*window, int key, int scancode, int action, int mods);
};


#endif //SANDBOX_VIEWPORT_H
