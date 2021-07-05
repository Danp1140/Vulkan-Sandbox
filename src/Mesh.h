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

typedef struct{
	glm::vec3 position, normal;
	glm::vec2 uv;
	std::string getString(){
		return "position: ("+std::to_string(position.x)+", "+std::to_string(position.y)+", "+std::to_string(position.z)+")\n"
				+"normal: <"+std::to_string(normal.x)+", "+std::to_string(normal.y)+", "+std::to_string(normal.z)+">\n"
				+"uv: ("+std::to_string(uv.x)+", "+std::to_string(uv.y)+")\n";
	}
}Vertex;
//IMPORTANT: both Edge and Tri structs contain index references to vertices vector....so, like, be aware/careful of that
typedef struct{
	unsigned int vertices[3];
}Tri;
typedef enum{
	MESH,
	RADIUS
}ShadowType;

class Mesh{
private:
	glm::vec3 position;
	std::vector<Vertex> vertices;
	std::vector<Tri> tris;
	glm::mat4 modelmatrix;
	ShadowType shadowtype;
	VkBuffer vertexbuffer;
	//maybe add shadowtype??? like dynamically calculated vs like "circle" or "blob" or "square"???

	void recalculateModelMatrix();
public:
	Mesh();
	Mesh(const char*filepath, glm::vec3 p);
	void draw(GLuint shaders, bool sendnormals, bool senduvs)const;
	glm::vec3 getPosition(){return position;}
	void setPosition(glm::vec3 p);
	std::vector<Vertex> getVertices(){return vertices;}
	std::vector<Tri> getTris(){return tris;}
	void loadOBJ(const char*filepath);
	void loadOBJLoadtimeOptimized(const char*filepath);
};


#endif //SANDBOX_MESH_H
