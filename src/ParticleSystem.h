//
// Created by Daniel Paavola on 8/13/21.
//

#ifndef VULKANSANDBOX_PARTICLESYSTEM_H
#define VULKANSANDBOX_PARTICLESYSTEM_H

#include "Mesh.h"

typedef enum ParticleType {
	POINTS,
	GRASS
} ParticleType;

typedef enum DistributionType {
	ENFORCED_UNIFORM_ON_MESH,
	UNIFORM_IN_SPACE
} DistributionType;

typedef union DistributionData {
	Mesh* mesh;
	glm::vec3 bounds[2];
} DistributionData;

class Particle {
public:
	glm::vec3 position;
	glm::mat4 modelmatrix;

	Particle (glm::vec3 p) {
		position = p;
//		modelmatrix=glm::mat4(
//				1., 0., 0., position.x,
//				0., 1., 0., position.y,
//				0., 0., 1., position.z,
//				0., 0., 0., 1.);
		modelmatrix = glm::translate(position);
	}

	virtual void generateUniqueData () {}

	virtual void generateTris (float seed, std::vector<Vertex>* v, std::vector<Tri>* t) {
		*v = std::vector<Vertex>();
		*t = std::vector<Tri>();
	}
};

class GrassParticle : public Particle {
public:
	float length, thickness, rigidity;  //add orientation later
	GrassParticle (glm::vec3 p) : Particle(p) {}

	void generateUniqueData () override {  //move up to init?
		length = 0.5;
		thickness = 0.01;
		rigidity = 0.5;
	}

	void generateTris (float seed, std::vector<Vertex>* v, std::vector<Tri>* t) override {
//		float templen=
//		(*v).push_back({
//			position+glm::vec3(thickness, 0.0, 0.0),
//			glm::vec3(0.0, 0.0, 1.0),
//			glm::vec2(1.0, 0.0)});
//		(*v).push_back({
//           position+glm::vec3(0.0, length, 0.0),
//           glm::vec3(0.0, 0.0, 1.0),
//           glm::vec2(0.5, 1.0)});
//		(*v).push_back({
//			position+glm::vec3(-thickness, 0.0, 0.0),
//			glm::vec3(0.0, 0.0, 1.0),
//			glm::vec2(0.0, 0.0)});
//		(*t).push_back({
//			{0, 0, 0},
//			{&(*v)[v->size()-3], &(*v)[v->size()-2], &(*v)[v->size()-1]},
//			std::vector<Tri*>(),
//	        glm::vec3(0.0, 0.0, 1.0)});
//		t->back().vertexindices[0]=v->size()-3;
//		t->back().vertexindices[1]=v->size()-2;
//		t->back().vertexindices[2]=v->size()-1;
	}
};

template<class T>
class ParticleSystem : public Mesh {
private:
	std::vector<T> particles;
	uint32_t numparticles;
	uint8_t variations;
	ParticleType type;
	DistributionType distributiontype;
	DistributionData distributiondata;
	VkBuffer* particleuniformbuffers;
	VkDeviceMemory* particleuniformbuffermemories;

	void distributeParticles ();

	void initDescriptorSets () override;

public:
	ParticleSystem (ParticleType t, uint32_t nparticles, DistributionType disttype, DistributionData distdata);

	// below creates a pipeline specialized for grass, re-evaluate as we expand the system
	static void createPipeline ();

	void recordDraw (uint8_t fifindex, uint8_t sciindex, VkDescriptorSet* sceneds) const;
};

#include "ParticleSystem.inl"

#endif //VULKANSANDBOX_PARTICLESYSTEM_H
