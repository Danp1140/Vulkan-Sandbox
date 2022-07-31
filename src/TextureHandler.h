//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"
#include <cstdarg>

#define ITERATE_2D_U32(xbound, ybound) for (uint32_t x = 0; x < xbound; x++) for (uint32_t y = 0; y < ybound; y++)

#define USE_FUNC_T T(* useFunc)(float_t, const BaseUseInfo<T>&)
#define USE_FUNC_DECL(funcname) \
        template<class T> \
        static T funcname(float_t value, const BaseUseInfo<T>& useinfo) \

#define GEN_FUNC_T float (* genFunc)(uint32_t, uint32_t, const TextureInfo&, BaseGenInfo&)
#define GEN_FUNC_DECL(funcname) static float funcname(uint32_t x, uint32_t y, const TextureInfo& texinfo, BaseGenInfo& geninfo)
#define GEN_FUNC_IMPL(funcname) float TextureHandler::funcname(uint32_t x, uint32_t y, const TextureInfo& texinfo, BaseGenInfo& geninfo)

#define TEX_FUNC_T(funcname, T) void (* funcname)(TextureInfo&, T*)
typedef struct TexGenFuncSet {
	TEX_FUNC_T(diffuse, glm::vec4);

	TEX_FUNC_T(normal, glm::vec4);

	TEX_FUNC_T(height, float);
} TexGenFuncSet;
#define TEX_FUNC_DECL(funcname, textype, datatype) static void funcname##textype(TextureInfo& texdst, datatype* datadst)
#define TEX_FUNC_DECL_DIFFUSE(funcname) TEX_FUNC_DECL(funcname, Diffuse, glm::vec4)
#define TEX_FUNC_DECL_NORMAL(funcname) TEX_FUNC_DECL(funcname, Normal, glm::vec4)
#define TEX_FUNC_DECL_HEIGHT(funcname) TEX_FUNC_DECL(funcname, Height, float)
#define TEX_FUNC_SET_DECL(funcname) \
        TEX_FUNC_DECL_DIFFUSE(funcname); \
        TEX_FUNC_DECL_NORMAL(funcname); \
        TEX_FUNC_DECL_HEIGHT(funcname); \
        static const TexGenFuncSet constexpr funcname##Set {funcname##Diffuse, funcname##Normal, funcname##Height} \

#define TEX_FUNC_IMPL(funcname, textype, datatype) void TextureHandler::funcname##textype(TextureInfo& texdst, datatype* datadst)
#define TEX_FUNC_IMPL_DIFFUSE(funcname) TEX_FUNC_IMPL(funcname, Diffuse, glm::vec4)
#define TEX_FUNC_IMPL_NORMAL(funcname) TEX_FUNC_IMPL(funcname, Normal, glm::vec4)
#define TEX_FUNC_IMPL_HEIGHT(funcname) TEX_FUNC_IMPL(funcname, Height, float)

template<class T>
struct InterpUseInfo {
	T endpoints[2];
};

// TODO: add cutoff use

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
	// TODO: add linear/radial and angle for linear
} WaveGenInfo;

typedef enum RandomType {
	RANDOM_TYPE_UNIFORM
} RandomType;

typedef union RandomDistribution {
	std::uniform_real_distribution<float_t> uniform;
} RandomDistribution;

typedef struct RandomGenInfo {
	RandomType type;
	RandomDistribution distribution;
} RandomGenInfo;

typedef union BaseGenInfo {
	WaveGenInfo wave;
	RandomGenInfo random;
} BaseGenInfo;

class TextureHandler {
private:
	static std::default_random_engine randomengine;
	static float** turbulence;
	static uint32_t turbulenceresolution;

	// TODO: look into templating these to make alloc simpler
	static void* allocTex (const TextureInfo& texinfo);

	static void deallocTex (void* ptr);

//	template<class T>
//	static void add (
//			const TextureInfo& texinfo,
//			T* dst,
//			GEN_FUNC_T,
//			BaseGenInfo& geninfo,
//			USE_FUNC_T,
//			const BaseUseInfo<T>& useinfo) {
//		for (uint32_t x = 0; x < texinfo.resolution.width; x++)
//			for (uint32_t y = 0; y < texinfo.resolution.height; y++) {
//				addTexel(dst[x * texinfo.resolution.height + y])
//			}
//	}

	template<class T>
	static void addTexel (
			const TextureInfo& texinfo,
			T& dst,
			GEN_FUNC_T,
			BaseGenInfo& geninfo,
			USE_FUNC_T,
			const BaseUseInfo<T>& useinfo,
			uint32_t x,
			uint32_t y) {
		dst += useFunc(genFunc(x, y, texinfo, geninfo), useinfo);
	}

	USE_FUNC_DECL(interp) {
		// TODO: add more interp options than just linear
		return value * useinfo.interp.endpoints[0] + (1.f - value) * useinfo.interp.endpoints[1];
	}

	GEN_FUNC_DECL(generateWave);

	GEN_FUNC_DECL(generateRandom);

	GEN_FUNC_DECL(generateTurbulence);

public:
	TextureHandler ();

	static void init ();

	// TODO: make texdsts pass by reference
	static void generateTextures (std::vector<TextureInfo> texdsts, const TexGenFuncSet funcset);

	TEX_FUNC_SET_DECL(generateMacroTest);

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
