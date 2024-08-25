//
// Created by Daniel Paavola on 7/31/21.
//

#ifndef VULKANSANDBOX_TEXTUREHANDLER_H
#define VULKANSANDBOX_TEXTUREHANDLER_H

#include "GraphicsHandler.h"
#include "PhysicsHandler.h"
#include <cstdarg>

/*
 * Macros
 */

#define ITERATE_2D_U32(xbound, ybound) for (uint32_t x = 0; x < xbound; x++) for (uint32_t y = 0; y < ybound; y++)

#define USE_FUNC_T T(* useFunc)(float_t, BaseUseInfo<T>&)
#define USE_FUNC_DECL(funcname) \
        template<class T> \
        static T funcname(float_t value, BaseUseInfo<T>& useinfo) \

#define GEN_FUNC_T float (* genFunc)(uint32_t, uint32_t, const TextureInfo&, BaseGenInfo&)
#define ANONYMOUS_GEN_FUNC_T float (*)(uint32_t, uint32_t, const TextureInfo&, BaseGenInfo&)
#define GEN_FUNC_DECL(funcname) static float funcname(uint32_t x, uint32_t y, const TextureInfo& texinfo, BaseGenInfo& geninfo)
#define GEN_FUNC_IMPL(funcname) float TextureHandler::funcname(uint32_t x, uint32_t y, const TextureInfo& texinfo, BaseGenInfo& geninfo)

#define TEX_FUNC_T(funcname, T) void (* funcname)(TextureInfo&, T*, void*)
typedef struct TexGenFuncSet {
	TEX_FUNC_T(diffuse, glm::vec4);

	TEX_FUNC_T(normal, glm::vec4);

	TEX_FUNC_T(height, float);
} TexGenFuncSet;
#define TEX_FUNC_DECL(funcname, textype, datatype) static void funcname##textype(TextureInfo& texdst, datatype* datadst, void* genvars)
#define TEX_FUNC_DECL_DIFFUSE(funcname) TEX_FUNC_DECL(funcname, Diffuse, glm::vec4)
#define TEX_FUNC_DECL_NORMAL(funcname) TEX_FUNC_DECL(funcname, Normal, glm::vec4)
#define TEX_FUNC_DECL_HEIGHT(funcname) TEX_FUNC_DECL(funcname, Height, float)
#define TEX_FUNC_SET_DECL(funcname) \
        TEX_FUNC_DECL_DIFFUSE(funcname); \
        TEX_FUNC_DECL_NORMAL(funcname); \
        TEX_FUNC_DECL_HEIGHT(funcname); \
        static const TexGenFuncSet constexpr funcname##TexGenSet {funcname##Diffuse, funcname##Normal, funcname##Height} \

#define TEX_FUNC_IMPL(funcname, textype, datatype) void TextureHandler::funcname##textype(TextureInfo& texdst, datatype* datadst, void* genvars)
#define TEX_FUNC_IMPL_DIFFUSE(funcname) TEX_FUNC_IMPL(funcname, Diffuse, glm::vec4)
#define TEX_FUNC_IMPL_NORMAL(funcname) TEX_FUNC_IMPL(funcname, Normal, glm::vec4)
#define TEX_FUNC_IMPL_HEIGHT(funcname) TEX_FUNC_IMPL(funcname, Height, float)

/*
 * Use Info Structs
 */

template<class T>
struct InterpUseInfo {
	T endpoints[2];
};

template<class T>
struct CutoffUseInfo {
	float cutoff;
	T low, high;
};

typedef struct ColorMapUseInfo {
	uint8_t numcolors;
	std::vector<glm::vec4> colors;
} ColorMapUseInfo;

template<class T>
union BaseUseInfo {
	InterpUseInfo<T> interp;
	ColorMapUseInfo colormap;
	CutoffUseInfo<T> cutoff;

	// TODO: fix this destructor
	~BaseUseInfo () {}
};

/* 
 * Gen Info Structs
 */

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

typedef struct VoronoiGenInfo {
	size_t numpoints;
	std::vector<glm::uvec2> points = {}; // initialized on first gen call
	// mindist is minimum distance required between two members of VGI.points
	// maxdist is the maximum possible distance a pixel can have from a member of VGI.points
	// for normalization
	float mindist = 0, maxdist = 0;
} VoronoiGenInfo;

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
	VoronoiGenInfo voronoi;

	BaseGenInfo() {}
	~BaseGenInfo() {}
} BaseGenInfo;

typedef struct CoarseRockSetVars {
	// int1 is coarser, int2 is finer
	glm::vec4 basecolor1, basecolor2, intrusioncolor1, intrusioncolor2;
} CoarseRockSetVars;

class TextureHandler {
private:
	static std::default_random_engine randomengine;
	static float** turbulence;
	static uint32_t turbulenceresolution;

	// TODO: look into templating these to make alloc simpler
	static void* allocTex (const TextureInfo& texinfo);
	static void deallocTex (void* ptr);

	/*
	 * Texel Access Functions
	 */
	template<class T>
	static void addTexel (
			const TextureInfo& texinfo,
			T& dst,
			GEN_FUNC_T,
			BaseGenInfo& geninfo,
			USE_FUNC_T,
			BaseUseInfo<T>& useinfo,
			uint32_t x,
			uint32_t y) {
		dst += useFunc(genFunc(x, y, texinfo, geninfo), useinfo);
	}

	template<class T>
	static void setTexel (
			const TextureInfo& texinfo,
			T& dst,
			std::vector<ANONYMOUS_GEN_FUNC_T>& genfuncs,
			std::vector<BaseGenInfo>& geninfos,
			USE_FUNC_T,
			BaseUseInfo<T>& useinfo,
			uint32_t x, uint32_t y) {
		float temp = 0;
		for (uint8_t i = 0; i < genfuncs.size(); i++) {
			temp += genfuncs[i](x, y, texinfo, geninfos[i]);
		}
		dst = useFunc(temp, useinfo);
	}

	template<class T>
	static void alphaBlendTexel (
			const TextureInfo& texinfo,
			T& dst,
			std::vector<ANONYMOUS_GEN_FUNC_T>& genfuncs,
			std::vector<BaseGenInfo>& geninfos,
			USE_FUNC_T,
			BaseUseInfo<T>& useinfo,
			uint32_t x, uint32_t y) {
		float temp = 0;
		for (uint8_t i = 0; i < genfuncs.size(); i++) {
			temp += genfuncs[i](x, y, texinfo, geninfos[i]);
		}
		glm::vec4 outtexel = useFunc(temp, useinfo);
		dst = outtexel * outtexel.w + dst * (1 - outtexel.w);
	}

	/*
	 * Use Functions
	 */
	USE_FUNC_DECL(interp) {
		return value * useinfo.interp.endpoints[0] + (1.f - value) * useinfo.interp.endpoints[1];
	}

	USE_FUNC_DECL(cutoff) {
		return value > useinfo.cutoff.cutoff ? useinfo.cutoff.high : useinfo.cutoff.low;
	}

	USE_FUNC_DECL(colorMap) {
		if (useinfo.colormap.colors.size() == 0) {
			std::uniform_real_distribution unidist(0., 1.);
			for (uint8_t i = 0; i < useinfo.colormap.numcolors; i++) {
				useinfo.colormap.colors.push_back(glm::vec4(
					unidist(randomengine),
					unidist(randomengine),
					unidist(randomengine),
					1.));
			}
		}
		return useinfo.colormap.colors[uint(value * useinfo.colormap.numcolors) % useinfo.colormap.numcolors];
	}

	/*
	 * Gen Functions
	 */
	GEN_FUNC_DECL(generateWave);
	GEN_FUNC_DECL(generateRandom);
	GEN_FUNC_DECL(generateVoronoi);
	GEN_FUNC_DECL(generateTurbulence);

public:
	TextureHandler ();

	static void init ();

	// TODO: should probably be vector of TexInfo ptrs
	static void generateTextures (std::vector<TextureInfo>&& texdsts, const TexGenFuncSet& funcset, void* genvars);

	/*
	 * Texture Function Sets
	 */
	TEX_FUNC_SET_DECL(macroTest);
	TEX_FUNC_SET_DECL(grid);
	TEX_FUNC_SET_DECL(colorfulMarble);
	TEX_FUNC_SET_DECL(ocean);
	TEX_FUNC_SET_DECL(coarseRock);
	TEX_FUNC_SET_DECL(blank);

	static void generateGraniteTextures (TextureInfo** texdsts, uint8_t numtexes);

	static void generateSkyboxTexture (TextureInfo* texdst);

	static float linearInterpolatedNoise (glm::vec2 T, uint32_t N, float** noise);

	static float turbulentNoise (glm::ivec2 T, uint32_t N, uint32_t reslev, float** noise);

	static glm::vec4 normalizedColorRamp (float x);

	// TODO: add mipmap generation and normal from height generation
};

#endif //VULKANSANDBOX_TEXTUREHANDLER_H
