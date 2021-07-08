//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_MESH_H
#define SANDBOX_MESH_H

#include <vector>
#include <map>
#include <iostream>
#include <random>
#include <cstdarg>

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>

typedef struct Vertex{
	glm::vec3 position, normal;
	glm::vec2 uv;
	std::string getString(){
		return "position: ("+std::to_string(position.x)+", "+std::to_string(position.y)+", "+std::to_string(position.z)+")\n"
				+"normal: <"+std::to_string(normal.x)+", "+std::to_string(normal.y)+", "+std::to_string(normal.z)+">\n"
				+"uv: ("+std::to_string(uv.x)+", "+std::to_string(uv.y)+")\n";
	}
}Vertex;
//IMPORTANT: both Edge and Tri structs contain index references to vertices vector....so, like, be aware/careful of that
typedef struct Tri{
	//recently changed this from unsigned int to uint16_t, which may be a source of trouble
	uint16_t vertices[3];
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
}TextureInfo;

typedef struct PipelineInfo{
	VkDescriptorSetLayout descriptorsetlayout;
	VkBuffer uniformbuffer;
	VkDeviceMemory uniformbuffermemory;
	VkPipelineLayout pipelinelayout;
	VkPipeline pipeline;
} PipelineInfo;

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
	PipelineInfo primarygraphicspipeline;
	VkFramebuffer*framebuffers;
	VkCommandPool commandpool;
	VkCommandBuffer*commandbuffers, interimcommandbuffer;
	VkSemaphore imageavailablesemaphore, renderfinishedsemaphore;
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingbuffermemory;
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
	VkCommandBuffer commandbuffer;
	std::vector<TextureInfo> textures;
	static VulkanInfo*vulkaninfo;
	//maybe add shadowtype??? like dynamically calculated vs like "circle" or "blob" or "square"???

	void recalculateModelMatrix();
	void updateBuffers();
	void initCommandBuffer();
	static void copyBuffer(VkBuffer*src, VkBuffer*dst, VkDeviceSize size, VkCommandBuffer*cmdbuff);
	static void copyBufferToImage(VkBuffer*src, VkImage*dst, uint32_t resolution);
	static void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldlayout, VkImageLayout newlayout);
public:
	Mesh(VulkanInfo*vki);
	Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki);
	~Mesh();
	void draw(GLuint shaders, bool sendnormals, bool senduvs)const;
	glm::vec3 getPosition(){return position;}
	void setPosition(glm::vec3 p);
	std::vector<Vertex> getVertices(){return vertices;}
	std::vector<Tri> getTris(){return tris;}
	const VkCommandBuffer*getCommandBuffer()const{return &commandbuffer;}
	void addTexture(uint32_t resolution, void*data);
	void loadOBJ(const char*filepath);
	void loadOBJLoadtimeOptimized(const char*filepath);
	static void createAndAllocateBuffer(VkBuffer*buffer, VkDeviceSize buffersize, VkBufferUsageFlags bufferusage, VkDeviceMemory*buffermemory, VkMemoryPropertyFlags reqprops);
};


#endif //SANDBOX_MESH_H
