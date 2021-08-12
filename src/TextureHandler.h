//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"

class TextureHandler{
private:
	std::default_random_engine randomengine;
//	std::lognormal_distribution<float> lognormdist;
public:
	TextureHandler();
	//maybe don't make static
	void generateStaticSandTextures(
			glm::vec4 sedimentdiffusegradient[2],
			uint32_t hres,
			uint32_t vres,
			TextureInfo*diffusetex,
			TextureInfo*normaltex);
	void generateMonotoneTextures(
			glm::vec4 color,
			uint32_t hres,
			uint32_t vres,
			TextureInfo*diffusetex,
			TextureInfo*normaltex);
	void generateOceanTextures(
			uint32_t hres,
			uint32_t vres,
			TextureInfo*heighttex,
			TextureInfo*normaltex);
	void updateOceanTextures(
			uint32_t hres,
			uint32_t vres,
			TextureInfo*heighttex,
			TextureInfo*normaltex);
	void generateVec4MipmapData(uint32_t numlevels,
									   uint32_t res,
									   glm::vec4**data);
};


#endif //VULKANSANDBOX_TEXTUREHANDLER_H
