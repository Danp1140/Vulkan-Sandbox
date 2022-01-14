//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_MESH_H
#define SANDBOX_MESH_H

#include "GraphicsHandler.h"

class Mesh{
private:
	glm::mat4 modelmatrix;
	TextureInfo diffusetexture, normaltexture, heighttexture;
	static VulkanInfo*vulkaninfo;

	void initCommandBuffers();
	void recalculateModelMatrix();
protected:
	glm::vec3 position, scale, min, max;
	glm::quat rotation;
	std::vector<Vertex> vertices;
	std::vector<Tri> tris;
	VkBuffer vertexbuffer, indexbuffer, *uniformbuffers;
	VkDeviceMemory vertexbuffermemory, indexbuffermemory, *uniformbuffermemories;
	VkDescriptorSet*descriptorsets;
	VkCommandBuffer*commandbuffers, **shadowcommandbuffers;
	MeshUniformBuffer uniformbufferdata;
	//can we automate updatebuffers calls?
	void updateBuffers();
	virtual void initDescriptorSets();
	void texInit(uint32_t dir, uint32_t nir, uint32_t hir);
	void triangulatePolygon(std::vector<glm::vec2>v, std::vector<glm::vec2>&dst);
public:
	Mesh(VulkanInfo*vki);
	Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki, uint32_t dir, uint32_t nir, uint32_t hir);
	Mesh(glm::vec3 p);
	~Mesh();
	virtual void rewriteTextureDescriptorSets();
	virtual void recordDraw(uint8_t fifindex, uint8_t sciindex, VkDescriptorSet*sceneds)const;
	virtual void recordShadowDraw(uint8_t fifindex, uint8_t sciindex, VkRenderPass renderpass, VkFramebuffer framebuffer, uint8_t lightidx, ShadowmapPushConstants*pc)const;
	void generateSteppeMesh(std::vector<glm::vec3>area, std::vector<std::vector<glm::vec3>>waters, double seed);
	glm::vec3 getPosition(){return position;}
	void setPosition(glm::vec3 p);
	glm::quat getRotation(){return rotation;}
	void setRotation(glm::quat r);
	glm::vec3 getScale(){return scale;}
	void setScale(glm::vec3 s);
	std::vector<Vertex> getVertices(){return vertices;}
	std::vector<Tri> getTris()const{return tris;}
	std::vector<Tri>*getTrisPtr(){return &tris;}
	glm::vec3 getMin(){return min;}
	glm::vec3 getMax(){return max;}
	VkCommandBuffer*getCommandBuffers()const{return commandbuffers;}
	VkCommandBuffer**getShadowCommandBuffers()const{return shadowcommandbuffers;}
	VkDescriptorSet*getDescriptorSets()const{return descriptorsets;}
	TextureInfo*getDiffuseTexturePtr(){return &diffusetexture;}
	TextureInfo*getNormalTexturePtr(){return &normaltexture;}
	TextureInfo*getHeightTexturePtr(){return &heighttexture;}
	VkDeviceMemory*getUniformBufferMemories()const{return uniformbuffermemories;}
	MeshUniformBuffer getUniformBufferData();
	void cleanUpVertsAndTris();
	void loadOBJ(const char*filepath);
};


#endif //SANDBOX_MESH_H
