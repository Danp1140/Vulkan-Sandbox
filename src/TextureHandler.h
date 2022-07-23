//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"
#include <cstdarg>

#define TEX_GEN_FUNC_DECL(funcname) static void funcname (TextureInfo** texdsts, uint8_t numtexes);

template<class T>
struct WaveGenInfo {
	glm::mat3 transform;
	T modulatees[2];
};

typedef union BaseGenInfo {
	WaveGenInfo<float> floatwave;
	WaveGenInfo<glm::vec4> vec4wave;
} BaseGenInfo;

typedef void (* baseGenFunc) (void*, uint32_t, uint32_t, const TextureInfo&, const BaseGenInfo&);

class TextureHandler {
private:
	std::default_random_engine randomengine;
	float** turbulence;
	uint32_t turbulenceresolution;

	static void* allocTex (const TextureInfo& texinfo);

	static void deallocTex (void* ptr);

	template<class T>
	static void add (const TextureInfo& texinfo, T* dst, baseGenFunc genfunc, const BaseGenInfo& geninfo) {
		T* temp = new T;
		for (uint32_t x = 0; x < texinfo.resolution.width; x++) {
			for (uint32_t y = 0; y < texinfo.resolution.height; y++) {
				genfunc(reinterpret_cast<void*>(temp), x, y, texinfo, geninfo);
				dst[x * texinfo.resolution.height + y] += temp;
			}
		}
		delete temp;
	}

	static void generateSineTextures (
			void* dst,
			uint32_t x,
			uint32_t y,
			const TextureInfo& texinfo,
			const BaseGenInfo& geninfo);

public:
	TextureHandler ();

	void generateNewSystemTextures (
			std::vector<TextureInfo&> texdsts
	);

	void generateGridTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	void generateSandTextures (
			TextureInfo** texdsts,
			uint8_t numtexes,
			glm::vec4* sedimentdiffusegradient,
			std::vector<Tri> tris,
			glm::vec3(* windVVF) (glm::vec3));

	void generateSteppeTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	void generateTestTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	void generateOceanTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	static void generateBlankTextures (
			TextureInfo** texdsts,
			uint8_t numtexes);

	void generateSkyboxTexture (TextureInfo* texdst);

	void generateVec4MipmapData (
			uint32_t numlevels,
			uint32_t res,
			glm::vec4** data);

	float linearInterpolatedNoise (glm::vec2 T, uint32_t N, float** noise);

	float turbulentNoise (glm::ivec2 T, uint32_t N, uint32_t reslev, float** noise);

	void
	generateNormalFromHeight (float** src, glm::vec4** dst, uint32_t resh, uint32_t resn, float worldspacetotexspace);

	glm::vec4 normalizedColorRamp (float x);

	void normalizedColorMap (float** src, glm::vec4** dst, uint32_t dim, uint32_t numcols);
};

#endif //VULKANSANDBOX_TEXTUREHANDLER_H
