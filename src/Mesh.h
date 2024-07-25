//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_MESH_H
#define SANDBOX_MESH_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"
#include "TextureHandler.h"

#define NO_LOADING_BARS

typedef enum RockType {
	ROCK_TYPE_GRANITE
} RockType;

typedef struct PrimaryGraphicsPushConstants {
	glm::mat4 cameravpmatrices;
	alignas(16) glm::vec3 camerapos;
	alignas(16) glm::vec2 standinguv;
	uint32_t numlights;
} PrimaryGraphicsPushConstants;

class Mesh {
private:
	TextureInfo diffusetexture, normaltexture, heighttexture;

	void recalculateModelMatrix ();

protected:
	glm::vec3 position, scale, min, max;
	glm::quat rotation;
	std::vector<Vertex> vertices;
	std::vector<Tri> tris;
	VkBuffer vertexbuffer = VK_NULL_HANDLE, indexbuffer = VK_NULL_HANDLE, * uniformbuffers;
	VkDeviceMemory vertexbuffermemory, indexbuffermemory, * uniformbuffermemories;
	VkDescriptorSet* descriptorsets = nullptr;
	MeshUniformBuffer uniformbufferdata;
	std::mutex dsmutex;

	virtual void initDescriptorSets ();

	void texInit (uint32_t dir, uint32_t nir, uint32_t hir);

	void triangulatePolygon (std::vector<glm::vec3> v, std::vector<glm::vec3>& dst);

	void subdivide (uint8_t levels);

	// TODO: add generateSmoothVertexNormals function
	void generateSmoothVertexNormals ();

public:
	PrimaryGraphicsPushConstants pcdata;

	Mesh (
			glm::vec3 p = glm::vec3(0.f),
			glm::vec3 s = glm::vec3(1.f),
			glm::quat r = glm::quat(0.f, 0.f, 0.f, 1.f),
			uint32_t dir = 64u,
			uint32_t nir = 64u,
			uint32_t hir = 64u);


	Mesh (
			const char* filepath,
			glm::vec3 p = glm::vec3(0.f),
			glm::vec3 s = glm::vec3(1.f),
			glm::quat r = glm::quat(0.f, 0.f, 0.f, 1.f),
			uint32_t dir = 64u,
			uint32_t nir = 64u,
			uint32_t hir = 64u);

	~Mesh ();

	virtual void rewriteTextureDescriptorSets ();
	static void createPipeline ();
	static void createShadowmapPipeline ();
	static void recordDraw (cbRecData data, VkCommandBuffer& cb);
	static void recordShadowDraw (cbRecData data, VkCommandBuffer& cb);
	void generateSteppeMesh (std::vector<glm::vec3> area, std::vector<std::vector<glm::vec3>> waters, double seed);
	glm::vec3 getPosition () {return position;}
	void setPosition (glm::vec3 p);
	glm::quat getRotation () {return rotation;}
	void setRotation (glm::quat r);
	glm::vec3 getScale () {return scale;}
	void setScale (glm::vec3 s);
	std::vector<Vertex> getVertices () {return vertices;}
	const std::vector<Tri>& getTris () const {return tris;}
	std::vector<Tri>* getTrisPtr () {return &tris;}
	glm::vec3 getMin () {return min;}
	glm::vec3 getMax () {return max;}
	// TODO: don't give out pointers unless /absolutely/ neccesary, its unsafe and lame
	VkDescriptorSet* getDescriptorSets () const {return descriptorsets;}

	TextureInfo* getDiffuseTexturePtr () {return &diffusetexture;}

	TextureInfo* getNormalTexturePtr () {return &normaltexture;}

	TextureInfo* getHeightTexturePtr () {return &heighttexture;}

	VkDeviceMemory* getUniformBufferMemories () const {return uniformbuffermemories;}

	VkBuffer getVertexBuffer () {return vertexbuffer;}

	VkBuffer getIndexBuffer () {return indexbuffer;}

	std::mutex* getDSMutexPtr () {return &dsmutex;}

	MeshUniformBuffer getUniformBufferData ();

	/* loadOBJ and cleanUpVertsAndTris are both used for clean filling of vertices and tris with data in preparation for
	 * the creation of the vertex and index buffers. loadOBJ loads every vertex and implied triangle from an obj file,
	 * and cleanUpVertsAndTris gets rid of redundancies and fills much of the triangles' members (notably it is used in
	 * more contexts than loadOBJ).
	 */
	void loadOBJ (const char* filepath);

	void cleanUpVertsAndTris ();

	void makeIntoIcosphere ();

	void makeIntoCube ();

	static Mesh* generateBoulder (RockType type, glm::vec3 scale, uint seed);

	void transform (glm::mat4 m);

	void proportionalTransform (glm::mat4 m, glm::vec3 o, float_t r);
};


#endif //SANDBOX_MESH_H
