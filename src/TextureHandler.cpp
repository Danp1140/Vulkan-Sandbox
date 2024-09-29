//
// Created by Daniel Paavola on 7/31/21.
//

#include <math.h>
#include "TextureHandler.h"

std::default_random_engine TextureHandler::randomengine {};
float** TextureHandler::turbulence = nullptr;
uint32_t TextureHandler::turbulenceresolution = -1u;

TextureHandler::TextureHandler () {}

void TextureHandler::init () {
	// TODO: use list init
	randomengine = std::default_random_engine();
	std::uniform_real_distribution unidist(0., 1.);
	const uint32_t randgridres = 16;
	// TODO: break up below into functions
	float** randgrid = new float* [randgridres];
	for (uint32_t x = 0; x < randgridres; x++) {
		randgrid[x] = new float[randgridres];
		for (uint32_t y = 0; y < randgridres; y++) {
			randgrid[x][y] = unidist(randomengine);
		}
	}
	turbulenceresolution = 2048;
	turbulence = new float* [turbulenceresolution];
	for (uint32_t y = 0; y < turbulenceresolution; y++) {
		turbulence[y] = new float[turbulenceresolution];
		memset(turbulence[y], 0, turbulenceresolution);
	}
	float max = 0.f, min = MAXFLOAT;
	for (uint32_t y = 0; y < turbulenceresolution; y++) {
		for (uint32_t x = 0; x < turbulenceresolution; x++) {
			turbulence[x][y] = turbulentNoise(
					glm::ivec2(x, y),
					randgridres,
					turbulenceresolution,
					randgrid);
			if (turbulence[x][y] > max) max = turbulence[x][y];
			if (turbulence[x][y] < min) min = turbulence[x][y];
		}
	}
	for (uint32_t y = 0; y < turbulenceresolution; y++) {
		for (uint32_t x = 0; x < turbulenceresolution; x++) {
			turbulence[x][y] = turbulence[x][y] / max + min / max;
		}
	}
	for (uint32_t x = 0; x < randgridres; x++) delete[] randgrid[x];
	delete[] randgrid;
}

void* TextureHandler::allocTex (const TextureInfo& texinfo) {
	if (texinfo.type == TEXTURE_TYPE_CUBEMAP) {
		return malloc(texinfo.resolution.width * texinfo.resolution.height *
					  GraphicsHandler::VKHelperGetPixelSize(texinfo.format) * 6);
	}
	return malloc(texinfo.resolution.width * texinfo.resolution.height *
				  GraphicsHandler::VKHelperGetPixelSize(texinfo.format));
}

void TextureHandler::deallocTex (void* ptr) {
	free(ptr);
}

void TextureHandler::generateTextures (std::vector<TextureInfo>&& texdsts, const TexGenFuncSet& funcset, void* genvars) {
	for (auto& t: texdsts) {
		// TODO: change these reinterpret_casts to static_casts
		// TODO: change hard-coded, type-based pixel sizes to use VKHelperGetPixelSize from format
		if (t.type == TEXTURE_TYPE_DIFFUSE) {
			glm::vec4* data = reinterpret_cast<glm::vec4*>(allocTex(t));
			memset(reinterpret_cast<void*>(data), 0, t.resolution.width * t.resolution.height * sizeof(glm::vec4));
			funcset.diffuse(t, data, genvars);
			GraphicsHandler::VKHelperUpdateWholeTexture(t, reinterpret_cast<void*>(data));
			deallocTex(reinterpret_cast<void*>(data));
		} else if (t.type == TEXTURE_TYPE_NORMAL) {
			glm::vec4* data = reinterpret_cast<glm::vec4*>(allocTex(t));
			memset(reinterpret_cast<void*>(data), 0, t.resolution.width * t.resolution.height * sizeof(glm::vec4));
			funcset.normal(t, data, genvars);
			GraphicsHandler::VKHelperUpdateWholeTexture(t, reinterpret_cast<void*>(data));
			deallocTex(reinterpret_cast<void*>(data));
		} else if (t.type == TEXTURE_TYPE_HEIGHT) {
			float* data = reinterpret_cast<float*>(allocTex(t));
			memset(reinterpret_cast<void*>(data), 0, t.resolution.width * t.resolution.height * sizeof(float));
			funcset.height(t, data, genvars);
			GraphicsHandler::VKHelperUpdateWholeTexture(t, reinterpret_cast<void*>(data));
			deallocTex(reinterpret_cast<void*>(data));
		} else if (t.type == TEXTURE_TYPE_SPECULAR) {
			float* data = reinterpret_cast<float*>(allocTex(t));
			memset(reinterpret_cast<void*>(data), 0, t.resolution.width * t.resolution.height * GraphicsHandler::VKHelperGetPixelSize(t.format));
			funcset.specular(t, data, genvars);
			GraphicsHandler::VKHelperUpdateWholeTexture(t, reinterpret_cast<void*>(data));
			deallocTex(reinterpret_cast<void*>(data));
		}
	}
}

GEN_FUNC_IMPL(generateWave) {
	WaveGenInfo gi = geninfo.wave;
	if (gi.type == WAVE_TYPE_SINE) return sin((x + gi.offset) / gi.wavelength * 6.28);
	else if (gi.type == WAVE_TYPE_TRI)
		return -abs(fmod((x + gi.offset) / gi.wavelength, 1.f) * 2.f - 1.f) + 1.f;
	else if (gi.type == WAVE_TYPE_SAW) return abs(fmod((x + gi.offset) / gi.wavelength, 1.f));
	else if (gi.type == WAVE_TYPE_SQUARE)
		return (fmod((x + gi.offset) / gi.wavelength, 1.f) > 0.5f) ? 1.f : 0.f;
	return 0.f;
}

GEN_FUNC_IMPL(generateRandom) {
	switch (geninfo.random.type) {
		case RANDOM_TYPE_UNIFORM:
		default:
			return geninfo.random.distribution.uniform(randomengine);
	}
}

GEN_FUNC_IMPL(generateVoronoi) {
	// ptr so we actually end up editing the data
	VoronoiGenInfo* gi = &geninfo.voronoi;
	glm::ivec2 pointtemp;
	bool tooclose;
	// assumes square tex...
	std::uniform_real_distribution<float> unidist(0, (float)texinfo.resolution.width);
	float disttemp;
	while (gi->points.size() != gi->numpoints) {
		pointtemp = glm::ivec2(unidist(randomengine), unidist(randomengine));
		tooclose = false;
		gi->maxdist = 0;
		for (const glm::uvec2& p : gi->points) {
			disttemp = sqrt(pow((float)pointtemp.x - (float)p.x, 2) + pow((float)pointtemp.y - (float)p.y, 2));
			if (disttemp <= gi->mindist) {
				tooclose = true;
				break;
			}
			disttemp /= 2;
			if (disttemp > gi->maxdist) gi->maxdist = disttemp;
		}
		if (!tooclose) {
			gi->points.push_back(pointtemp);
		}
	}

	// super close, but this doesnt quite tile seamlessly
	float closestdist = std::numeric_limits<float>::infinity();
	for (const glm::uvec2& p : gi->points) {
		pointtemp = glm::ivec2(x, y);
		disttemp = sqrt(pow((float)pointtemp.x - (float)p.x, 2) + pow((float)pointtemp.y - (float)p.y, 2));
		if (disttemp < closestdist) closestdist = disttemp;

		if (x * 2 < texinfo.resolution.width) {
			pointtemp.x = (int)x + (int)texinfo.resolution.width;
		}
		else {
			pointtemp.x = (int)x - (int)texinfo.resolution.width;
		}
		if (y * 2 < texinfo.resolution.height) {
			pointtemp.y = (int)y + (int)texinfo.resolution.height;
		}
		else { 
			pointtemp.y = (int)y - (int)texinfo.resolution.height;
		}
		disttemp = sqrt(pow((float)pointtemp.x - (float)p.x, 2) + pow((float)pointtemp.y - (float)p.y, 2));
		if (disttemp < closestdist) closestdist = disttemp;
		disttemp = sqrt(pow((float)x - (float)p.x, 2) + pow((float)pointtemp.y - (float)p.y, 2));
		if (disttemp < closestdist) closestdist = disttemp;
		disttemp = sqrt(pow((float)pointtemp.x - (float)p.x, 2) + pow((float)y - (float)p.y, 2));
		if (disttemp < closestdist) closestdist = disttemp;
	}
	return closestdist / gi->maxdist;
}

GEN_FUNC_IMPL(generateTurbulence) {
	return turbulence
		[(size_t)floor((float)x / (float)(texinfo.resolution.width - 1) * (float)(turbulenceresolution - 1))]
		[(size_t)floor((float)y / (float)(texinfo.resolution.height - 1) * (float)(turbulenceresolution - 1))];
}

TEX_FUNC_IMPL_DIFFUSE(macroTest) {
	BaseGenInfo geninfo;
	geninfo.wave = {WAVE_TYPE_SINE, texdst.resolution.width / 25.f, 0.f};
	BaseUseInfo<glm::vec4> useinfo {.interp = {glm::vec4(0.f, 0.f, 0.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 1.f)}};
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
			geninfo.wave.offset = 100.f * generateTurbulence(x, y, texdst, geninfo);
			addTexel(texdst,
					 datadst[x * texdst.resolution.height + y],
					 generateWave,
					 geninfo,
					 interp,
					 useinfo,
					 x, y);
		}
}

TEX_FUNC_IMPL_NORMAL(macroTest) {}

TEX_FUNC_IMPL_HEIGHT(macroTest) {}

TEX_FUNC_IMPL_DIFFUSE(grid) {
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
			// TODO: add grid density param
			if (y % 32 == 0 || x % 32 == 0) datadst[x * texdst.resolution.height + y] = glm::vec4(0.f, 0.f, 0.f, 1.f);
			else datadst[x * texdst.resolution.height + y] = glm::vec4(1.f, 1.f, 1.f, 1.f);
		}
}

TEX_FUNC_IMPL_NORMAL(grid) {}

TEX_FUNC_IMPL_HEIGHT(grid) {}

TEX_FUNC_IMPL_DIFFUSE(colorfulMarble) {
	BaseGenInfo geninfo {};
	BaseUseInfo<glm::vec4> useinfo {.colormap = {16u, {}}};
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
			geninfo.wave = {WAVE_TYPE_SAW,
							(float_t)texdst.resolution.width / 25.f,
							100.f * generateTurbulence(x, y, texdst, geninfo)};
			addTexel(texdst, datadst[x * texdst.resolution.height + y], generateWave, geninfo, colorMap, useinfo, x, y);
		}
}

TEX_FUNC_IMPL_NORMAL(colorfulMarble) {}

TEX_FUNC_IMPL_HEIGHT(colorfulMarble) {}

TEX_FUNC_IMPL_SPECULAR(colorfulMarble) {}

TEX_FUNC_IMPL_DIFFUSE(ocean) {}

TEX_FUNC_IMPL_NORMAL(ocean) {
	BaseGenInfo geninfo {};
	BaseUseInfo<glm::vec4> useinfo {.interp = {{glm::vec4(1., 1., 0., 0.), glm::vec4(-1., 1., 0., 0)}}};
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
		geninfo.wave = {WAVE_TYPE_TRI, 20.f, 100.f * generateTurbulence(x, y, texdst, geninfo)};
		datadst[x * texdst.resolution.height + y] = glm::vec4(0);
		// perhaps we need a setTexel method
		addTexel(texdst, datadst[x * texdst.resolution.height + y], generateWave, geninfo, interp, useinfo, x, y);
	}
}

TEX_FUNC_IMPL_HEIGHT(ocean) {}

TEX_FUNC_IMPL_DIFFUSE(coarseRock) {
	CoarseRockSetVars vars = genvars ? *static_cast<CoarseRockSetVars*>(genvars) : (CoarseRockSetVars){};
	BaseUseInfo<glm::vec4> useinfos[3] {
		{.interp = {vars.basecolor1, vars.basecolor2}},
		{.cutoff = {0.9, glm::vec4(0), vars.intrusioncolor1}},
		{.cutoff = {0.9, glm::vec4(0), vars.intrusioncolor2}}
	};
	std::vector<ANONYMOUS_GEN_FUNC_T> genfuncs[3] = {
		{generateVoronoi},
		{generateVoronoi, generateTurbulence},
		{generateTurbulence}
	};
	std::vector<BaseGenInfo> geninfos[3];
	geninfos[0] = std::vector<BaseGenInfo>(1);
	geninfos[1] = std::vector<BaseGenInfo>(2);
	geninfos[2] = std::vector<BaseGenInfo>(1);
	geninfos[0][0].voronoi = {32, {}, 1.};
	geninfos[1][0].voronoi = {32, {}, 1.};
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
		setTexel(
			texdst, 
			datadst[x * texdst.resolution.height + y], 
			genfuncs[0],
			geninfos[0],
			interp,
			useinfos[0],
			x, y);
		alphaBlendTexel(
			texdst, 
			datadst[x * texdst.resolution.height + y], 
			genfuncs[1],
			geninfos[1],
			cutoff,
			useinfos[1],
			x, y);
		alphaBlendTexel(
			texdst, 
			datadst[x * texdst.resolution.height + y], 
			genfuncs[2],
			geninfos[2],
			cutoff,
			useinfos[2],
			(x + 500) % texdst.resolution.width, (y + 500) % texdst.resolution.height);
	}
}

TEX_FUNC_IMPL_NORMAL(coarseRock) {
	RandomDistribution r = {.uniform = std::uniform_real_distribution<float_t>(0, 1)};
	std::vector<ANONYMOUS_GEN_FUNC_T> genfuncs = {generateRandom};
	std::vector<BaseGenInfo> geninfos(1);
	geninfos[0].random = {RANDOM_TYPE_UNIFORM, r};
	BaseUseInfo<glm::vec4> useinfo = {.interp = {glm::vec4(0.1, 0.1, 0, 1), glm::vec4(-0.1, 0.1, 0, 1)}};
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
		setTexel(
			texdst,
			datadst[x * texdst.resolution.height + y],
			genfuncs,
			geninfos,
			interp,
			useinfo,
			x, y);
	}
}

TEX_FUNC_IMPL_HEIGHT(coarseRock) {
	RandomDistribution r = {.uniform = std::uniform_real_distribution<float_t>(0, 1)};
	std::vector<ANONYMOUS_GEN_FUNC_T> genfuncs = {generateRandom};
	std::vector<BaseGenInfo> geninfos(1);
	geninfos[0].random = {RANDOM_TYPE_UNIFORM, r};
	BaseUseInfo<float> useinfo = {.interp = {0, 0.05}};
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
		setTexel(
			texdst,
			datadst[x * texdst.resolution.height + y],
			genfuncs,
			geninfos,
			interp,
			useinfo,
			x, y);
	}
}

TEX_FUNC_IMPL_SPECULAR(coarseRock) {
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
		datadst[x * texdst.resolution.height + y] = 64;
	}
}

TEX_FUNC_IMPL_DIFFUSE(blank) {
	ITERATE_2D_U32(texdst.resolution.width, texdst.resolution.height) {
			datadst[x * texdst.resolution.height + y] = glm::vec4(1.f);
		}
}

TEX_FUNC_IMPL_NORMAL(blank) {}

TEX_FUNC_IMPL_HEIGHT(blank) {}

TEX_FUNC_IMPL_SPECULAR(blank) {}

void TextureHandler::generateGraniteTextures (TextureInfo** texdsts, uint8_t numtexes) {
	// granite may have several differnet colors
	// granite may be spotty, splotchy, or veiny, depending on relationship between face and vein
	// unpolished boulders tend towards spotty
	// consider layering brownian cutoff textures
	const uint32_t scale = 100; // i.e. num oscillations, TODO: add as function arg
	// what if we offset the x and y waves???
	float_t wavelength;
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			wavelength = texdsts[texidx]->resolution.width / (float_t)scale; // assumes square texture
			std::uniform_real_distribution unidist(0., 1.);
			glm::vec4* finaltex = (glm::vec4*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
			float** turbulentsquare = new float* [texdsts[texidx]->resolution.width];
			for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
				turbulentsquare[x] = new float[texdsts[texidx]->resolution.height];
				for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
					turbulentsquare[x][y] = 0.9f;
				};
			}
			for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
				for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
					if (turbulence[x % turbulenceresolution][y % turbulenceresolution] < 0.5) {
						finaltex[x + y * texdsts[texidx]->resolution.width] = glm::vec4(0., 0., 0., 1.);
					} else {
						finaltex[x + y * texdsts[texidx]->resolution.width] = glm::vec4(1.);
					}
				}
			}
			GraphicsHandler::VKHelperUpdateWholeTexture(*texdsts[texidx], reinterpret_cast<void*>(finaltex));
			for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) delete[] turbulentsquare[x];
			delete[] turbulentsquare;
			delete[] finaltex;
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_NORMAL ||
				   texdsts[texidx]->type == TEXTURE_TYPE_HEIGHT) {
			size_t size =
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4);
			glm::vec4* data = (glm::vec4*)malloc(size);
			memset(data, 0, size);
			GraphicsHandler::VKHelperUpdateWholeTexture(*texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		}
	}
}

void TextureHandler::generateSkyboxTexture (TextureInfo* texdst) {
	//pass in sun for position info and color info diffusion
	glm::vec4* data = new glm::vec4[texdst->resolution.width * texdst->resolution.height * sizeof(glm::vec4) *
									6];
	glm::vec3 directionvectortemp;
	glm::vec2 uv;
	for (uint32_t layer = 0; layer < 6; layer++) {
		glm::vec4* tempptr = data + layer * texdst->resolution.height * texdst->resolution.width;
//		normalizedColorMap(turbulence, &tempptr, 2048, 16);
		for (uint32_t y = 0; y < texdst->resolution.height; y++) {
			for (uint32_t x = 0; x < texdst->resolution.width; x++) {
				uv = glm::vec2((float)x / (float)texdst->resolution.width,
							   1. - (float)y / (float)texdst->resolution.height);
				if (layer == 0) directionvectortemp = glm::vec3(1., uv.y * 2. - 1., (1. - uv.x) * 2. - 1.);
				else if (layer == 1) directionvectortemp = glm::vec3(-1., uv.y * 2 - 1., uv.x * 2. - 1.);
				else if (layer == 2) directionvectortemp = glm::vec3(uv.x * 2. - 1., 1., (1 - uv.y) * 2. - 1.);
				else if (layer == 3) directionvectortemp = glm::vec3(uv.x * 2. - 1., -1., uv.y * 2. - 1.);
				else if (layer == 4) directionvectortemp = glm::vec3(uv.x * 2. - 1., uv.y * 2. - 1., 1.);
				else if (layer == 5)
					directionvectortemp = glm::vec3((1. - uv.x) * 2. - 1.,
													uv.y * 2. - 1.,
													-1.);
				else directionvectortemp = glm::vec3(0.);
				directionvectortemp = glm::normalize(directionvectortemp);
				if (glm::length(directionvectortemp - glm::normalize(glm::vec3(0., 100., 500))) < 0.05) {
					data[layer * texdst->resolution.height * texdst->resolution.width +
						 y * texdst->resolution.width +
						 x] = glm::vec4(1.);
				} else {
//					if(directionvectortemp.y<0.) data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x]=glm::vec4(0.3, 0.3, 0.7, 1.);
//					if(directionvectortemp.y<0.) data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x]=glm::vec4(0.5, 0.5, 0.5, 1.);
//					if(directionvectortemp.y<0.) data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x]=glm::vec4(1., 0., 0., 1.);
//					if(directionvectortemp.y<0.) data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x]=glm::vec4(directionvectortemp.x, 1.+directionvectortemp.y, directionvectortemp.z, 1.);
//					else{
					const float R = 6371., h = 20.;
					const glm::vec3 P = glm::vec3(0., R, 0.), L = directionvectortemp;
					const float b = 2 * glm::dot(P, L);
					float t = (-b + sqrt(b * b + 8. * R * h + 4. * h * h)) / 2.;
					data[layer * texdst->resolution.height * texdst->resolution.width +
						 y * texdst->resolution.width +
						 x] = glm::vec4((1. - directionvectortemp.y) * 0.5,
										(1. - directionvectortemp.y) * 0.5,
										0.9,
										1.);
//						float lambda;
//						for(uint8_t i=0;i<3;i++){
//							lambda=i==0?750.:(i==1?500.:450.);
//							lambda*=pow(10., -9.);
//							std::cout<<lambda<<'\t'<<t<<std::endl;
//							for(uint32_t d=t/5.;d>0;d--){
//								std::cout<<pow(lambda, -4.)<<std::endl;
//								data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x][i]
//									+=(8.*pow(3.14, 4.)*pow(2.118*pow(10., -29.), 1.))
//											/(pow(lambda, 4.))
//											*(1.+pow(glm::dot(glm::normalize(glm::vec3(0., 100., 500.)), L), 2.));
					//approximating sun w/ infinite distance, just normalize the sun's position
//							}
//						}
//						data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x].a=1.;
//						PRINT_QUAD(data[layer*texdst->resolution.height*texdst->resolution.width+y*texdst->resolution.width+x]);
//					}
				}
			}
		}
	}
	GraphicsHandler::VKHelperUpdateWholeTexture(*texdst, data);
	delete[] data;
}

float TextureHandler::linearInterpolatedNoise (glm::vec2 T, uint32_t N, float** noise) {
	//consider 2D catmull-rom splines later
	// ^^^ what does this mean lol???
	glm::ivec2 TI = glm::vec2((int)T.x, (int)T.y);
	glm::vec2 dT = T - (glm::vec2)TI;
	TI = glm::ivec2(TI.x % N, TI.y % N);
	if (T.x >= 0 && T.y >= 0) {
		float xinterpa = dT.x * noise[(TI.x + 1) % N][TI.y] + (1.0 - dT.x) * noise[TI.x][TI.y],
				xinterpb =
				dT.x * noise[(TI.x + 1) % N][(TI.y + 1) % N] + (1.0 - dT.x) * noise[TI.x][(TI.y + 1) % N];
		return dT.y * xinterpb + (1.0 - dT.y) * xinterpa;
	}
	return 0;
}

float TextureHandler::turbulentNoise (glm::ivec2 T, uint32_t N, uint32_t reslev, float** noise) {
	float result = 0., scalingfactor = (float)N / (float)reslev;
	for (uint32_t x = 1; x < reslev; x *= 2) {
		result += abs(linearInterpolatedNoise(
				glm::vec2((float)T.x * scalingfactor * (float)x, (float)T.y * scalingfactor * (float)x),
				N,
				noise) / (float)x);
	}
	return result;
}

glm::vec4 TextureHandler::normalizedColorRamp (float x) {
	return glm::vec4(1., 0., 0., 1.) * sin(x * 3.14)
		   + glm::vec4(0., 1., 0., 1.) * abs(sin(x * 3.14 + (3.14 / 3.)))
		   + glm::vec4(0., 0., 1., 1.) * abs(sin(x * 3.14 + (6.28 / 3.)));
}
