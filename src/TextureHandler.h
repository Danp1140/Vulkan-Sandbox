//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"

class TextureHandler{
private:
	std::default_random_engine randomengine;
	float**turbulence;
	uint32_t turbulenceresolution;
public:
	TextureHandler ();

	void generateGridTextures (
			TextureInfo**texdsts,
			uint8_t numtexes);

	void generateSandTextures (
			TextureInfo**texdsts,
			uint8_t numtexes,
			glm::vec4*sedimentdiffusegradient,
			std::vector<Tri> tris,
			glm::vec3(*windVVF) (glm::vec3));

	void generateSteppeTextures (
			TextureInfo**texdsts,
			uint8_t numtexes);

	void generateTestTextures (
			TextureInfo**texdsts,
			uint8_t numtexes);

	void generateOceanTextures (
			TextureInfo**texdsts,
			uint8_t numtexes);

	static void generateBlankTextures (
			TextureInfo**texdsts,
			uint8_t numtexes);

	void generateSkyboxTexture (TextureInfo*texdst);

	void generateVec4MipmapData (
			uint32_t numlevels,
			uint32_t res,
			glm::vec4**data);

	float linearInterpolatedNoise (glm::vec2 T, uint32_t N, float**noise);

	float turbulentNoise (glm::ivec2 T, uint32_t N, uint32_t reslev, float**noise);

	void
	generateNormalFromHeight (float**src, glm::vec4**dst, uint32_t resh, uint32_t resn, float worldspacetotexspace);

	glm::vec4 normalizedColorRamp (float x);

	void normalizedColorMap (float**src, glm::vec4**dst, uint32_t dim, uint32_t numcols);
};


#endif //VULKANSANDBOX_TEXTUREHANDLER_H
