//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"

class TextureHandler {
private:
	static std::default_random_engine randomengine;
	static float** turbulence;
	static uint32_t turbulenceresolution;
public:
	TextureHandler ();

	static void init ();

	static void generateGridTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	static void generateSandTextures (
			TextureInfo** texdsts,
			uint8_t numtexes,
			glm::vec4* sedimentdiffusegradient,
			std::vector<Tri> tris,
			glm::vec3(* windVVF) (glm::vec3));

	static void generateSteppeTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	static void generateTestTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	static void generateOceanTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	static void generateBlankTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	static void generateGraniteTextures (TextureInfo** texdsts, uint8_t numtexes);

	static void generateSkyboxTexture (TextureInfo* texdst);

	static void generateVec4MipmapData (
			uint32_t numlevels,
			uint32_t res,
			glm::vec4** data);

	static float linearInterpolatedNoise (glm::vec2 T, uint32_t N, float** noise);

	static float turbulentNoise (glm::ivec2 T, uint32_t N, uint32_t reslev, float** noise);

	static void
	generateNormalFromHeight (float** src, glm::vec4** dst, uint32_t resh, uint32_t resn, float worldspacetotexspace);

	static glm::vec4 normalizedColorRamp (float x);

	static void normalizedColorMap (float** src, glm::vec4** dst, uint32_t dim, uint32_t numcols);
};


#endif //VULKANSANDBOX_TEXTUREHANDLER_H
