//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_MESH_H
#define SANDBOX_MESH_H

#include "GraphicsHandler.h"

typedef enum ShadowType{
	MESH,
	RADIUS
} ShadowType;

class Mesh{
private:
	glm::vec3 position;
	std::vector<Vertex> vertices;
	std::vector<Tri> tris;
	glm::mat4 modelmatrix;
	ShadowType shadowtype;
	VkBuffer vertexbuffer, indexbuffer;
	VkDeviceMemory vertexbuffermemory, indexbuffermemory;
	VkDescriptorSet*descriptorsets;
	std::vector<TextureInfo> textures;
	static VulkanInfo*vulkaninfo;
	//maybe add shadowtype??? like dynamically calculated vs like "circle" or "blob" or "square"???

	void recalculateModelMatrix();
	void updateBuffers();
	void initDescriptorSets();
public:
	Mesh(VulkanInfo*vki);
	Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki);
	~Mesh();
	glm::vec3 getPosition(){return position;}
	void setPosition(glm::vec3 p);
	std::vector<Vertex> getVertices(){return vertices;}
	std::vector<Vertex>*getVerticesPtr(){return &vertices;}
	std::vector<Tri> getTris()const{return tris;}
	std::vector<Tri>*getTrisPtr(){return &tris;}
	const VkBuffer*getVertexBuffer()const{return &vertexbuffer;}
	const VkBuffer*getIndexBuffer()const{return &indexbuffer;}
	const std::vector<TextureInfo> getTextures()const{return textures;}
	const VkDescriptorSet*getDescriptorSets()const{return &descriptorsets[0];}
	static VulkanInfo*getVulkanInfoPtr(){return vulkaninfo;}
	void loadOBJ(const char*filepath);
};


#endif //SANDBOX_MESH_H
