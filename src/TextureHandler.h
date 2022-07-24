//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"
#include <cstdarg>

#define TEX_GEN_FUNC_DECL(funcname) static void funcname (TextureInfo** texdsts, uint8_t numtexes);

#define USE_FUNC_T T(* useFunc)(float_t value, const BaseUseInfo<T>&)
#define GEN_FUNC_T float (* genFunc)(uint32_t, uint32_t, const TextureInfo&, const BaseGenInfo&)

template<class T>
struct InterpUseInfo {
	T endpoints[2];
};

template<class T>
union BaseUseInfo {
	InterpUseInfo<T> interp;
};

typedef enum WaveType {
	WAVE_TYPE_SINE,
	WAVE_TYPE_TRI,
	WAVE_TYPE_SAW,
	WAVE_TYPE_SQUARE
} WaveType;

typedef struct WaveGenInfo {
	WaveType type;
	float_t wavelength, offset;
} WaveGenInfo;

typedef union BaseGenInfo {
	WaveGenInfo wave;
} BaseGenInfo;

class TextureHandler {
private:
	static std::default_random_engine randomengine;
	static float** turbulence;
	static uint32_t turbulenceresolution;

	static void* allocTex (const TextureInfo& texinfo);

	static void deallocTex (void* ptr);

	template<class T>
	static void
	add (
			const TextureInfo& texinfo,
			T* dst,
			GEN_FUNC_T,
			const BaseGenInfo& geninfo,
			USE_FUNC_T,
			const BaseUseInfo<T>& useinfo) {
		for (uint32_t x = 0; x < texinfo.resolution.width; x++) {
			for (uint32_t y = 0; y < texinfo.resolution.height; y++) {
				dst[x * texinfo.resolution.height + y] += useFunc(genFunc(x, y, texinfo, geninfo), useinfo);
			}
		}
	}

	template<class T>
	static T interp (float_t value, const BaseUseInfo<T>& useinfo) {
		// TODO: add more interp options than just linear
		return value * useinfo.interp.endpoints[0] + (1.f - value) * useinfo.interp.endpoints[1];
	}

	static float generateWave (uint32_t x, uint32_t y, const TextureInfo& texinfo, const BaseGenInfo& geninfo);

public:
	TextureHandler ();

	static void init ();

	static void generateNewSystemTextures (std::vector<TextureInfo> texdsts);

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
