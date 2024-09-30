/* File: GraphicsHandler.h
 * Author: Daniel Paavola
 *
 * GraphicsHandler.h is intended to handle nearly all of the direct interaction with the Vulkan API, containing numerous
 * initialization functions and helper functions. It is designed to be referenced solely statically, as there is no
 * reason to have more than one of any of its members. It is at the very bottom of the include tree, soas to have its
 * member variables and classes available to nearly all other classes at any time.
 */

#ifndef SANDBOX_VIEWPORT_H
#define SANDBOX_VIEWPORT_H

#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <map>
#include <iostream>
#include <random>
#include <cstdarg>
#include <string>

#define VK_ENABLE_BETA_EXTENSIONS

#include <png/png.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan/vk_enum_string_helper.h>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>

#define GLFW_INCLUDE_VULKAN

#define ANSI_BLACK_FORE "\033[30m"
#define ANSI_RED_FORE "\033[31m"
#define ANSI_GREEN_FORE "\033[32m"
#define ANSI_BLUE_FORE "\033[34;1m"
#define ANSI_WHITE_FORE "\033[37;97m"
#define ANSI_RESET_FORE "\033[37m"

#define PRINT_DOUBLE(vec2) std::cout << '(' << vec2[0] << ", " << vec2[1] << ")\n"
#define PRINT_TRIPLE(vec3) std::cout << '(' << vec3[0] << ", " << vec3[1] << ", " << vec3[2] << ")\n"
#define PRINT_QUAD(vec4) std::cout << '(' << std::fixed << std::setprecision(2) << vec4[0] << ", " << vec4[1] << ", " << vec4[2] << ", " << vec4[3] << ")\n"
#define PRINT_4MAT(mat4) PRINT_QUAD(mat4[0]); PRINT_QUAD(mat4[1]); PRINT_QUAD(mat4[2]); PRINT_QUAD(mat4[3]);
#define SHIFT_DOUBLE(vec2) '(' << vec2[0] << ", " << vec2[1] << ')'
#define SHIFT_TRIPLE(vec3) '(' << vec3[0] << ", " << vec3[1] << ", " << vec3[2] << ')'
#define SHIFT_QUAD(vec4) '(' << vec4[0] << ", " << vec4[1] << ", " << vec4[2] << ", " << vec4[3] << ')'

#define WORKING_DIRECTORY "/Users/danp/Desktop/C Coding/VulkanSandbox/"
#define NUM_RECORDING_THREADS 1 // TODO: fix multithreading; changing this to > 1 seems to cause heap corruption
#define MAX_FRAMES_IN_FLIGHT 6 
#define SWAPCHAIN_IMAGE_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT
#define DEPTH_IMAGE_FORMAT VK_FORMAT_D32_SFLOAT
#define MAX_LIGHTS 2

#define VK_INIT_GUARD {if (GraphicsHandler::vulkaninfo.logicaldevice == VK_NULL_HANDLE) return;}

/*
 * Consts, Typedefs, and the Like
 */

#define NUM_SHADER_STAGES_SUPPORTED 5
const VkShaderStageFlagBits supportedshaderstages[NUM_SHADER_STAGES_SUPPORTED] = {
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT};

const char* const shaderstagestrs[NUM_SHADER_STAGES_SUPPORTED] = {
		"comp",
		"vert",
		"tesc",
		"tese",
		"frag"};

typedef enum ChangeFlagBits {
	NO_CHANGE_FLAG_BIT = 0x00000000,
	CAMERA_LOOK_CHANGE_FLAG_BIT = 0x00000001,
	CAMERA_POSITION_CHANGE_FLAG_BIT = 0x00000002,
	LIGHT_CHANGE_FLAG_BIT = 0x00000004,
} ChangeFlagBits;
typedef int ChangeFlag;

typedef enum TextureType {
	TEXTURE_TYPE_DIFFUSE,
	TEXTURE_TYPE_NORMAL,
	TEXTURE_TYPE_HEIGHT,
	TEXTURE_TYPE_DYNAMIC_HEIGHT,
	TEXTURE_TYPE_SPECULAR,
	TEXTURE_TYPE_CUBEMAP,
	TEXTURE_TYPE_SHADOWMAP,
	TEXTURE_TYPE_SCRATCH_BUFFER,
	TEXTURE_TYPE_SWAPCHAIN_DEPTH_BUFFER,
	TEXTURE_TYPE_CUSTOM, // allows usage & layout to pass through
	TEXTURE_TYPE_MAX
} TextureType;

// unsure of this, but defining a specific interim CB one too
const VkCommandBufferBeginInfo cmdbufferbegininfo {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		0,        // could make this one time submit???
		nullptr
};

const VkCommandBufferBeginInfo interimcbbegininfo {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
};

const VkDescriptorSetLayoutBinding scenedslbindings[2] {{
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		MAX_LIGHTS,
		VK_SHADER_STAGE_FRAGMENT_BIT |
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		nullptr
	}, {
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
}};

/*
 * Vulkan Type Wrappers
 */

typedef struct PipelineInitInfo {
	VkShaderStageFlags stages = 0u;
	const char* shaderfilepathprefix = nullptr;
	VkDescriptorSetLayoutCreateInfo* descsetlayoutcreateinfos = nullptr;
	VkPushConstantRange pushconstantrange = {};
	VkPipelineVertexInputStateCreateInfo vertexinputstatecreateinfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	bool depthtest = false;
	VkSpecializationInfo specinfo = {};
	VkRenderPass renderpass = VK_NULL_HANDLE; // defaults to primary via VKSubInitPipeline
	VkExtent2D extent = {0, 0}; // defaults to swapchain extent via VKSubInitPipeline
	VkCullModeFlags culling = VK_CULL_MODE_BACK_BIT;
	VkPrimitiveTopology topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // defaults to patch list via VKSubInitPipeline
									// if tess control is indicated in stages
} PipelineInitInfo;

typedef struct PipelineInfo {
	VkDescriptorSetLayout scenedsl = VK_NULL_HANDLE, objectdsl = VK_NULL_HANDLE;
	VkPipelineLayout pipelinelayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
} PipelineInfo;

typedef struct TextureInfo {
private:
	glm::vec2 scale, position;
	float rotation;

	void updateUVMatrix () {
		uvmatrix = glm::mat4(
				scale.x * cos(rotation), scale.y * sin(rotation), position.x, 0,
				-scale.x * sin(rotation), scale.y * cos(rotation), position.y, 0,
				0, 0, 1, 0,
				0, 0, 0, 0);
	}

public:
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView imageview = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags usage = 0x00000000;
	VkExtent2D resolution = {0, 0};
	VkMemoryPropertyFlags memoryprops = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	TextureType type = TEXTURE_TYPE_MAX;
	glm::mat3 uvmatrix = glm::mat3(1);
	VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D;
	uint32_t numlayers = 1;

	inline const VkDescriptorImageInfo getDescriptorImageInfo () const {
		return {sampler, imageview, layout};
	}

	inline const VkImageSubresourceRange getDefaultSubresourceRange () const {
		return {
			format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1, 0, numlayers
		};
	}

	void setUVScale (glm::vec2 s) {
		scale = s;
		updateUVMatrix();
	}

	void setUVRotation (float r) {
		rotation = r;
		updateUVMatrix();
	}

	void setUVPosition (glm::vec2 p) {
		position = p;
		updateUVMatrix();
	}
} TextureInfo;

typedef struct BufferInfo {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize elemsize = 0, roundedelemsize = 0;
	uint32_t numelems = 0;
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VkMemoryPropertyFlags memprops = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	inline const VkDescriptorBufferInfo getDescriptorBufferInfo() const {return {buffer, 0, VK_WHOLE_SIZE};}
} BufferInfo;

typedef struct SkyboxPushConstants {
	glm::mat4 cameravpmatrices;
	alignas(16) glm::vec3 sunposition;
} SkyboxPushConstants;

typedef struct ShadowmapPushConstants {
	glm::mat4 lightvpmatrices, lspmatrix;
} ShadowmapPushConstants;

/*
 * Uniform Buffer Structs
 */

typedef struct LightUniformBuffer {
	glm::mat4 vpmatrix, lspmatrix;
	glm::vec3 position;
	float intensity;
	glm::vec3 forward;
	uint32_t lighttype;
	glm::vec4 color;
} LightUniformBuffer;

typedef struct MeshUniformBuffer {
	glm::mat4 modelmatrix,
			diffuseuvmatrix,
			normaluvmatrix,
			heightuvmatrix;
} MeshUniformBuffer;

// TODO: clean up these typedefs to maximize efficiency
// clean up naming too

/*
 * Command Buffer Recording Infrastructure
 */

typedef struct cbRecData {
	VkRenderPass renderpass;
	VkFramebuffer framebuffer;
	VkDescriptorSet descriptorset, scenedescriptorset;
	PipelineInfo pipeline;
	void* pushconstantdata;
	std::mutex* dsmutex, * scenedsmutex;
	VkBuffer vertexbuffer, indexbuffer;
	uint16_t numtris;
} cbRecData;

typedef std::function<void (VkCommandBuffer&)> cbRecFunc;

typedef struct cbRecTask {
	cbRecTask () : 
		type(CB_REC_TASK_TYPE_UNINITIALIZED), 
		data() {}

	explicit cbRecTask (cbRecFunc f) : type(CB_REC_TASK_TYPE_COMMAND_BUFFER) {
		new(&data.func) cbRecFunc(f);
	}

	explicit cbRecTask (VkRenderPassBeginInfo r) : type(CB_REC_TASK_TYPE_RENDERPASS) {
		data.rpbi = r;
	}

	explicit cbRecTask (VkDependencyInfoKHR d) : type(CB_REC_TASK_TYPE_DEPENDENCY) {
		data.di = d;
	}

	cbRecTask (const cbRecTask& c) : type(c.type) {
		if (type == CB_REC_TASK_TYPE_COMMAND_BUFFER) new(&data.func) cbRecFunc(c.data.func);
		else if (type == CB_REC_TASK_TYPE_RENDERPASS) data.rpbi = c.data.rpbi;
		else data.di = c.data.di;
	}

	void operator= (const cbRecTask& c) {
		type = c.type;
		if (type == CB_REC_TASK_TYPE_COMMAND_BUFFER) new(&data.func) cbRecFunc(c.data.func);
		else if (type == CB_REC_TASK_TYPE_RENDERPASS) data.rpbi = c.data.rpbi;
		else data.di = c.data.di;
	}

	~cbRecTask () {
		if (type == CB_REC_TASK_TYPE_COMMAND_BUFFER) {
			data.func.~function();
		}
		type.~cbRecTaskType();
		data.~cbRecTaskData();
	}

	enum cbRecTaskType {
		CB_REC_TASK_TYPE_UNINITIALIZED,
		CB_REC_TASK_TYPE_COMMAND_BUFFER,
		CB_REC_TASK_TYPE_RENDERPASS,
		CB_REC_TASK_TYPE_DEPENDENCY,
	} type;

	union cbRecTaskData {
		cbRecTaskData () {}

		~cbRecTaskData () {} // union destructors are impossible to write...too bad!

		cbRecFunc func;
		VkRenderPassBeginInfo rpbi;
		VkDependencyInfoKHR di;
	} data;
} cbRecTask;

typedef struct cbCollectInfo {
	explicit cbCollectInfo (const VkCommandBuffer& c) : 
		type(CB_COLLECT_INFO_TYPE_COMMAND_BUFFER), 
		data {.cmdbuf = c} {}

	explicit cbCollectInfo (const VkRenderPassBeginInfo& r) : 
		type(CB_COLLECT_INFO_TYPE_RENDERPASS), 
		data {.rpbi = r} {}

	explicit cbCollectInfo (const VkDependencyInfoKHR& d) : 
		type(CB_COLLECT_INFO_TYPE_DEPENDENCY), 
		data {.di = d} {}

	enum cbCollectInfoType {
		CB_COLLECT_INFO_TYPE_COMMAND_BUFFER,
		CB_COLLECT_INFO_TYPE_RENDERPASS,
		CB_COLLECT_INFO_TYPE_DEPENDENCY
	} type;

	union cbCollectInfoData {
		VkCommandBuffer cmdbuf;
		VkRenderPassBeginInfo rpbi;
		VkDependencyInfoKHR di;
	} data;
} cbCollectInfo;

// this is one per thread, so no sync required :)
typedef struct cbSet {
	VkCommandPool pool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> buffers {};
} cbSet;

/*
 * Mega-Struct holding most of the data relevent to Vulkan goings-on (but not all of it).
 * This struct was originally created to allow passing a pointer to it around, but now
 * that GraphicsHandler members are so public, its barely neccesarry anymore.
 * TODO: figure out where all this stuff should really live lol
 */
typedef struct VulkanInfo {
	GLFWwindow* window;
	int horizontalres, verticalres, width, height;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugmessenger;
	VkPhysicalDevice physicaldevice;
	VkDevice logicaldevice = VK_NULL_HANDLE;
	VkQueue graphicsqueue, presentationqueue;
	uint32_t graphicsqueuefamilyindex, presentqueuefamilyindex, transferqueuefamilyindex;
	VkSwapchainKHR swapchain;
	uint32_t numswapchainimages;
	VkImage* swapchainimages;
	VkImageView* swapchainimageviews;
	VkExtent2D swapchainextent;     //  extent is redundant with hori/vertres variables
	VkRenderPass primaryrenderpass, templateshadowrenderpass, compositingrenderpass;
	PipelineInfo textgraphicspipeline,
			skyboxgraphicspipeline,
			grassgraphicspipeline,
			terraingencomputepipeline,
			voxeltroubleshootingpipeline,
			postprocpipeline,
			postprocgraphicspipeline;    // is it good to have this many pipelines? some can be combined if we want/need to
	VkFramebuffer* primaryframebuffers, * compositingframebuffers;
	VkClearValue primaryclears[2], shadowmapclear;
	VkCommandPool commandpool;
	cbSet threadCbSets[NUM_RECORDING_THREADS][MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer* commandbuffers, interimcommandbuffer;
	VkSemaphore* imageavailablesemaphores, renderfinishedsemaphores[MAX_FRAMES_IN_FLIGHT];
	// TODO: remove recordinginvalidfences
	VkFence* submitfinishedfences, * recordinginvalidfences, * presentfinishedfences, tempfence;
	int currentframeinflight;
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingbuffermemory;
	VkDescriptorPool descriptorpool;
	// TODO: delete
	VkDescriptorSetLayout scenedsl, defaultdsl, textdsl, oceangraphdsl, oceancompdsl, particledsl, shadowmapdsl, texmondsl, linesdsl;
	TextureInfo depthbuffer;
	VkBuffer* lightuniformbuffers;
	VkDeviceMemory* lightuniformbuffermemories;
	SkyboxPushConstants skyboxpushconstants;
	glm::mat4 grasspushconstants;
	glm::mat4 terrainpushconstants;
} VulkanInfo;

/*
 * Geometry Structs
 */

typedef struct Vertex {
	glm::vec3 position, normal;
	glm::vec2 uv;

	inline bool operator== (const Vertex& rhs) const {
		return (this->position == rhs.position && this->normal == rhs.normal && this->uv == rhs.uv);
	}
} Vertex;

typedef struct Tri {
	uint16_t vertexindices[3];
	Vertex* vertices[3];
	std::vector<Tri*> adjacencies;
	glm::vec3 algebraicnormal;
} Tri;

typedef struct KeyInfo {
	int currentvalue, lastvalue;
	bool up, down;
} KeyInfo;

const VkDescriptorSetLayoutCreateInfo scenedslcreateinfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		2,
		&scenedslbindings[0]
};

class GraphicsHandler {
private:
	/* Chain of initialization functions that set up the gritty inner workings of Vulkan and GLFW, including instance,
	 * device, queue, swapchain, descriptor pool, command pool, and pipeline initialization. Public VKInit below calls
	 * these in sequence.
	 * TODO: create destructor functions
	 */
	static void VKSubInitWindow ();
	static void VKSubInitInstance ();
	static void VKSubInitDebug ();
	static void VKSubInitDevices ();
	static void VKSubInitQueues ();
	static void VKSubInitSwapchain ();
	static void VKSubInitRenderpasses ();
	static void VKSubInitDescriptorLayoutsAndPool (uint32_t nummeshes);
	static void VKSubInitFramebuffers ();
	static void VKSubInitCommandPool ();
	static void VKSubInitSemaphoresAndFences ();
	static void VKSubInitSamplers ();

	// TODO: move the rest of the pipelines out of here to new system
	static void VKSubInitPipeline (
			PipelineInfo* pipelineinfo,
			VkShaderStageFlags shaderstages,
			const char** shaderfilepaths,
			VkDescriptorSetLayoutCreateInfo* descsetlayoutcreateinfos,
			VkPushConstantRange pushconstantrange,
			VkPipelineVertexInputStateCreateInfo vertexinputstatecreateinfo,
			bool depthtest,
			VkRenderPass renderpass);
	static void VKSubInitShaders (
			VkShaderStageFlags stages,
			const char** filepaths,
			VkShaderModule** modules,
			VkPipelineShaderStageCreateInfo** createinfos,
			VkSpecializationInfo* specializationinfos);

	static void VKSubInitDepthBuffer ();

public:
	// TODO: investigate best practices for handling of these vars. must be easily accesible, but could make read-only
	static VulkanInfo vulkaninfo;
	static ChangeFlag* changeflags;
	static std::stringstream troubleshootingsstrm;
	static std::map<int, KeyInfo> keyvalues;
	static uint32_t swapchainimageindex;
	/* A set of samplers to be used across descriptor set creations. Default endpoints are nearest for min/mag filters,
	 * repeat for all overmapping, and white border.
	 * TODO: make new VKSubInitSamplers for these, as they are currently hiding in VKInitPipelines
	 */
	static VkSampler genericsampler,
			linearminsampler,
			linearminmagsampler,
			clamptobordersampler,
			depthsampler;

	/*
	 * Constructors & Destructors
	 */
	GraphicsHandler ();
	~GraphicsHandler ();

	/*
	 * Initialization Functions (MUST BE CALLED BEFORE ANY OTHER GRAPHICS OBJECTS ARE INSTANTIATED)
	 */
	static void VKInit (uint32_t nummeshes);
	// TODO: destroy this function once and for all, cast it into the flames
	static void VKInitPipelines ();
	static void VKSubInitPipeline (PipelineInfo* pipelineinfo, PipelineInitInfo pii);
	static void destroyPipeline(PipelineInfo& p);

	/*
	 * Buffer Utilities
	 */
	static void VKHelperCreateAndAllocateBuffer (BufferInfo& b);
	static void VKSubInitUniformBuffer (
			VkDescriptorSet** descsets,
			uint32_t binding,
			VkDeviceSize elementsize,
			uint32_t elementcount,
			VkBuffer** buffers,
			VkDeviceMemory** memories,
			VkDescriptorSetLayout descsetlayout);
	static void VKSubInitUniformBuffer (BufferInfo& b);
	static void VKSubInitStorageBuffer (BufferInfo& b);
	static void VKHelperCreateAndAllocateBuffer (
			VkBuffer* buffer,
			VkDeviceSize buffersize,
			VkBufferUsageFlags bufferusage,
			VkDeviceMemory* buffermemory,
			VkMemoryPropertyFlags reqprops);
	static void VKHelperCpyBufferToBuffer (
			VkBuffer* src,
			const VkBuffer* const dst,
			VkDeviceSize size);
	static void VKHelperInitVertexAndIndexBuffers (
			const std::vector<Vertex>& vertices,
			const std::vector<Tri>& tris,
			VkBuffer* vertexbufferdst,
			VkDeviceMemory* vertexbuffermemorydst,
			VkBuffer* indexbufferdst,
			VkDeviceMemory* indexbuffermemorydst);
	static void VKHelperInitVertexBuffer (
			const std::vector<Vertex>& vertices,
			VkBuffer* vertexbufferdst,
			VkDeviceMemory* vertexbuffermemorydst);
	static void VKHelperUpdateUniformBuffer (
			uint32_t elementcount,
			VkDeviceSize elementsize,
			VkDeviceMemory buffermemory,
			void* data);
	// TODO: change below to UpdateDeviceLocal and UpdateHostCoherent cuz thats what they really are
	static void VKHelperUpdateUniformBuffer (const BufferInfo& b, void* data);
	static void VKHelperUpdateStorageBuffer (const BufferInfo& b, void* data);

	/*
	 * Image Utilities
	 */
	static void VKHelperCpyBufferToImage (
			VkBuffer* src,
			VkImage* dst,
			VkFormat format,
			uint32_t numlayers,
			uint32_t rowpitch,
			uint32_t horires, uint32_t vertres);
	static void transitionImageLayout (TextureInfo& t, VkImageLayout newlayout);
	static void createTexture (TextureInfo& t);
	static void destroyTexture (TextureInfo& t);
	static void VKHelperUpdateWholeTexture (TextureInfo& t, void* src);
	static void VKHelperRecordImageTransition (
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldlayout,
			VkImageLayout newlayout);
	static void recordImgCpy (cbRecData data, VkCopyImageInfo2 cpyinfo, VkCommandBuffer& cb);
	static void pipelineBarrierFromKHR (const VkDependencyInfoKHR& di);

	/* Expects all vectors to be of same length (and doesn't error check so could end up
	 * a bad access if you're not careful). Unneeded img or buf infos can be whatever, but 
	 * should be there as placeholders for indexing.
	 */
	static void updateDescriptorSet (
			const VkDescriptorSet ds, 
			const std::vector<uint32_t>& bindings,
			const std::vector<VkDescriptorType>& types,
			const std::vector<VkDescriptorImageInfo>& imginfos,
			const std::vector<VkDescriptorBufferInfo>& bufinfos);

	static void submitAndPresent ();

	// TODO: find the right place for these two functions (mat4TransfomVec3 and makeRectPrism) (maybe PhysicsHandler)
	static glm::vec3 mat4TransformVec3 (glm::mat4 M, glm::vec3 v);
	static glm::mat4 projectFromView (glm::mat4 P, glm::vec3 p, glm::vec3 c, glm::vec3 u);
	static void makeRectPrism (glm::vec3** dst, glm::vec3 min, glm::vec3 max);
	static VkDeviceSize VKHelperGetPixelSize (VkFormat format);

	/*
	 * Debug Utilities
	 */
	static VkResult CreateDebugUtilsMessengerEXT (
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* createinfo,
			const VkAllocationCallbacks* allocator,
			VkDebugUtilsMessengerEXT* debugutilsmessenger);
	static void DestroyDebugUtilsMessengerEXT (
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugutilsmessenger,
			const VkAllocationCallbacks* allocator);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callbackdata,
			void* userdata);

	/*
	 * GLFW Utilities
	 */
	static void glfwErrorCallback (int error, const char* description);
	static void keystrokeCallback (GLFWwindow* window, int key, int scancode, int action, int mods);
};


#endif //SANDBOX_VIEWPORT_H
