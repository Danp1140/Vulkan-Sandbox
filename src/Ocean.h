//
// Created by Daniel Paavola on 8/5/21.
//

#ifndef VULKANSANDBOX_OCEAN_H
#define VULKANSANDBOX_OCEAN_H

#include "Mesh.h"

#define OCEAN_PRE_TESS_SUBDIV 8

//why do we need "public"?
class Ocean:public Mesh{
private:
	glm::vec2 bounds;
	Mesh*shore;
	TextureInfo heightmap, normalmap;

	void initDescriptorSets();
public:
	Ocean(glm::vec3 pos, glm::vec2 b, Mesh*s);

	void rewriteAllDescriptorSets();
	void rewriteDescriptorSet(uint32_t index);
	TextureInfo*getHeightMapPtr(){return &heightmap;}
	TextureInfo*getNormalMapPtr(){return &normalmap;}
};


#endif //VULKANSANDBOX_OCEAN_H
