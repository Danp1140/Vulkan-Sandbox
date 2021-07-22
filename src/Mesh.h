//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_MESH_H
#define SANDBOX_MESH_H

#define GLFW_INCLUDE_VULKAN
#define PRINT_TRIPLE(vec3) std::cout<<'('<<vec3[0]<<", "<<vec3[1]<<", "<<vec3[2]<<")\n"
#define PRINT_QUAD(vec4) std::cout<<'('<<vec4[0]<<", "<<vec4[1]<<", "<<vec4[2]<<", "<<vec4[3]<<")"
#define MAX_FRAMES_IN_FLIGHT 2

#include <vector>
#include <map>
#include <iostream>
#include <random>
#include <cstdarg>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan/vk_enum_string_helper.h>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>

typedef struct Vertex{
	//tutorial gave idea of putting some simple functions for info like attribute bindings in struct
	glm::vec3 position, normal;
	glm::vec2 uv;
	inline bool operator==(const Vertex&rhs){
		return (this->position==rhs.position&&this->normal==rhs.normal&&this->uv==rhs.uv);
	}
}Vertex;
typedef struct Tri{
	//recently changed this from unsigned int to uint16_t, which may be a source of trouble
	uint16_t vertices[3];
	std::vector<Tri*> adjacencies;
	glm::vec3 algebraicnormal;
	//could also store an average vertex position
}Tri;
typedef enum ShadowType{
	MESH,
	RADIUS
} ShadowType;

typedef struct TextureInfo{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageview;
	VkSampler sampler;
	//consider making function inline
	VkDescriptorImageInfo getDescriptorImageInfo()const{
		return {
			sampler,
			imageview,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
	}
}TextureInfo;

typedef struct PipelineInfo{
	//could likely add push constants to this struct
	VkDescriptorSetLayout scenedescriptorsetlayout, meshdescriptorsetlayout;
	VkDescriptorSet*descriptorset;
	VkShaderModule vertexshadermodule, fragmentshadermodule;
	VkPipelineLayout pipelinelayout;
	VkPipeline pipeline;
} PipelineInfo;

typedef struct VulkanInfo{
	GLFWwindow*window;
	int horizontalres, verticalres, width, height;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugmessenger;
	VkPhysicalDevice physicaldevice;
	VkDevice logicaldevice;
	//better way to organize these?
	VkQueue graphicsqueue, presentationqueue;
	//perhaps should break up the bottom into however many uint32_ts, instead of one pointer/array
//	uint32_t*queuefamilyindices;
	uint32_t graphicsqueuefamilyindex, presentqueuefamilyindex, transferqueuefamilyindex;
	VkSwapchainKHR swapchain;
	uint32_t numswapchainimages;
	VkImage*swapchainimages;
	VkImageView*swapchainimageviews;
//  extent is redundant with hori/vertres variables
	VkExtent2D swapchainextent;
	VkRenderPass primaryrenderpass;
	PipelineInfo primarygraphicspipeline, textgraphicspipeline;
	VkFramebuffer*framebuffers;
	VkCommandPool commandpool;
	VkCommandBuffer*commandbuffers, interimcommandbuffer;
	VkCommandBufferInheritanceInfo*commandbufferinheritanceinfos;
	VkSemaphore imageavailablesemaphores[MAX_FRAMES_IN_FLIGHT], renderfinishedsemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence frameinflightfences[MAX_FRAMES_IN_FLIGHT], imageinflightfences[MAX_FRAMES_IN_FLIGHT];
	int currentframeinflight;
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingbuffermemory;
	VkDescriptorPool descriptorpool;
	VkDescriptorSetLayout textdescriptorsetlayout;
	//should we keep these buffers and memories in vulkaninfo, or as member variables of GraphicsHandler?
	VkBuffer*lightuniformbuffers;
	VkDeviceMemory*lightuniformbuffermemories;
} VulkanInfo;

class Mesh{
private:
	glm::vec3 position;
	std::vector<Vertex> vertices;
	std::vector<Tri> tris;
	glm::mat4 modelmatrix;
	ShadowType shadowtype;
	VkBuffer vertexbuffer, indexbuffer;
	VkDeviceMemory vertexbuffermemory, indexbuffermemory;
	VkCommandBuffer*commandbuffers;
	VkDescriptorSet*descriptorsets;
	std::vector<TextureInfo> textures;
	static VulkanInfo*vulkaninfo;
	//maybe add shadowtype??? like dynamically calculated vs like "circle" or "blob" or "square"???

	void recalculateModelMatrix();
	void updateBuffers();
	void initCommandBuffer();
	void recordCommandBuffer()const;
	void initDescriptorSets();
	static void copyBuffer(VkBuffer*src, VkBuffer*dst, VkDeviceSize size, VkCommandBuffer*cmdbuff);
	static void copyBufferToImage(VkBuffer*src, VkImage*dst, uint32_t horires, uint32_t vertres);
	static void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldlayout, VkImageLayout newlayout);
public:
	Mesh(VulkanInfo*vki);
	Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki);
	~Mesh();
	void draw(GLuint shaders, bool sendnormals, bool senduvs)const;
	glm::vec3 getPosition(){return position;}
	void setPosition(glm::vec3 p);
	std::vector<Vertex> getVertices(){return vertices;}
	std::vector<Vertex>*getVerticesPtr(){return &vertices;}
	std::vector<Tri> getTris()const{return tris;}
	std::vector<Tri>*getTrisPtr(){return &tris;}
	const VkBuffer*getVertexBuffer()const{return &vertexbuffer;}
	const VkBuffer*getIndexBuffer()const{return &indexbuffer;}
	const VkCommandBuffer*getCommandBuffers()const{return commandbuffers;}
	const std::vector<TextureInfo> getTextures()const{return textures;}
	const VkDescriptorSet*getDescriptorSets()const{return &descriptorsets[0];}
	static VulkanInfo*getVulkanInfoPtr(){return vulkaninfo;}
	static void addTexture(TextureInfo*texturedst,
						   uint32_t horires,
						   uint32_t vertres,
						   void*data,
						   uint32_t pixelsize,
						   VkFormat format);
	void loadOBJ(const char*filepath);
	static void createAndAllocateBuffer(VkBuffer*buffer, VkDeviceSize buffersize, VkBufferUsageFlags bufferusage, VkDeviceMemory*buffermemory, VkMemoryPropertyFlags reqprops);
};


#endif //SANDBOX_MESH_H
