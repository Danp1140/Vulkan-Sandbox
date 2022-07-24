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
	randomengine = std::default_random_engine();
	std::uniform_real_distribution unidist(0., 1.);
	const uint32_t randgridres = 16;
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
//			std::cout << turbulence[x][y] << std::endl;
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

float TextureHandler::generateWave (uint32_t x, uint32_t y, const TextureInfo& texinfo, const BaseGenInfo& geninfo) {
	WaveGenInfo gi = geninfo.wave;
	if (gi.type == WAVE_TYPE_SINE) return sin((x + gi.offset) / gi.wavelength * 6.28);
	else if (gi.type == WAVE_TYPE_TRI)
		return -abs(fmod((x + gi.offset) / gi.wavelength, 1.f) * 2.f - 1.f) + 1.f;
	else if (gi.type == WAVE_TYPE_SAW) return abs(fmod((x + gi.offset) / gi.wavelength, 1.f));
	else if (gi.type == WAVE_TYPE_SQUARE) return (fmod((x + gi.offset) / gi.wavelength, 1.f) > 0.5f) ? 1.f : 0.f;
}

void TextureHandler::generateNewSystemTextures (std::vector<TextureInfo> texdsts) {
	BaseGenInfo geninfo {};
	for (auto& t: texdsts) {
		if (t.type == TEXTURE_TYPE_DIFFUSE) {
			BaseUseInfo<glm::vec4> useinfo {};
			glm::vec4* data = reinterpret_cast<glm::vec4*>(allocTex(t));
			memset(reinterpret_cast<void*>(data), 0, t.resolution.width * t.resolution.height * sizeof(glm::vec4));
			useinfo.interp = {{glm::vec4(0.f, 1.f, 0.f, 1.f), glm::vec4(0.f, 0.f, 1.f, 1.f)}};
			geninfo.wave = {WAVE_TYPE_TRI,
							float_t(t.resolution.width) / 25.f,
							0.f};
			add(t, data, generateWave, geninfo, interp, useinfo);
			GraphicsHandler::VKHelperUpdateWholeTexture(&t, reinterpret_cast<void*>(data));
			deallocTex(reinterpret_cast<void*>(data));
		}
	}
}

void TextureHandler::generateGridTextures (
		TextureInfo** texdsts,
		uint8_t numtexes) {
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			glm::vec4* finaltex = (glm::vec4*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
			for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
				for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
					if (y % 32 == 0 || x % 32 == 0)
						finaltex[y * texdsts[texidx]->resolution.width + x] = glm::vec4(0., 0., 0., 1.);
					else finaltex[y * texdsts[texidx]->resolution.width + x] = glm::vec4(1., 1., 1., 1.);

				}
			}
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(finaltex));
			delete[] finaltex;
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_NORMAL || texdsts[texidx]->type == TEXTURE_TYPE_HEIGHT) {
			size_t size = texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4);
			glm::vec4* data = (glm::vec4*)malloc(size);
			memset(data, 0, size);
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		}
	}
}

void TextureHandler::generateSandTextures (
		TextureInfo** texdsts,
		uint8_t numtexes,
		glm::vec4* sedimentdiffusegradient,
		std::vector<Tri> tris,
		glm::vec3(* windVVF) (glm::vec3)) {
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			std::lognormal_distribution lognormdist(0.5, 1.0);
			float value;
			glm::vec4* data = (glm::vec4*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
			for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
				for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
					value = fmin(lognormdist(randomengine), 1.0);
					data[y * texdsts[texidx]->resolution.width + x] =
							sedimentdiffusegradient[1] * value + sedimentdiffusegradient[0] * (1.0 - value);
				}
			}
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_NORMAL) {
			glm::vec4* data = (glm::vec4*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
			memset(data, 0.,
				   texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
//			for(uint32_t y=0;y<texdsts[texidx]->resolution.height;y++){
//				for(uint32_t x=0;x<texdsts[texidx]->resolution.width;x++){
//					data[y*texdsts[texidx]->resolution.width+x]=glm::vec4(
//							0.,
//							-1.,
//							0.2*1./texdsts[texidx]->resolution.height*10.*cos((float)y/texdsts[texidx]->resolution.height*10.),
//							0.);
//				}
//			}
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_HEIGHT) {
			float* data = (float*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(float));
//			memset(data, 0., texdsts[texidx]->resolution.width*texdsts[texidx]->resolution.height*sizeof(float));
			for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
				for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
					data[y * texdsts[texidx]->resolution.width + x]
							= 0.05 * sin((float)y / (float)texdsts[texidx]->resolution.height * 100. +
										 turbulence[x % turbulenceresolution][y % turbulenceresolution] * 25.)
							  + 0.05 * cos((float)x / (float)texdsts[texidx]->resolution.width * 100. +
										   turbulence[x % turbulenceresolution][y % turbulenceresolution] * 25.);
				}
			}
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		}
	}
}

void TextureHandler::generateSteppeTextures (
		TextureInfo** texdsts,
		uint8_t numtexes) {
	TextureInfo* currenttex;
	size_t currenttexsize;
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		currenttex = texdsts[texidx];
		currenttexsize = currenttex->resolution.width * currenttex->resolution.height * sizeof(float);
		if (currenttex->type == TEXTURE_TYPE_DIFFUSE) {
			glm::vec4* data = new glm::vec4[currenttexsize];
			for (size_t i = 0; i < currenttexsize; i++) data[i] = glm::vec4(0.9, 0.9, 0.9, 1.);
			GraphicsHandler::VKHelperUpdateWholeTexture(currenttex, reinterpret_cast<void*>(data));
			delete[] data;
		}
		if (currenttex->type == TEXTURE_TYPE_NORMAL) {
			glm::vec4* data = new glm::vec4[currenttexsize];
			for (size_t i = 0; i < currenttexsize; i++) data[i] = glm::vec4(0., 1., 0., 0.);
			GraphicsHandler::VKHelperUpdateWholeTexture(currenttex, reinterpret_cast<void*>(data));
			delete[] data;
		}
		if (currenttex->type == TEXTURE_TYPE_HEIGHT) {
			float* data = new float[currenttexsize];
			memset(data, 0., currenttexsize);
			GraphicsHandler::VKHelperUpdateWholeTexture(currenttex, reinterpret_cast<void*>(data));
			delete[] data;
		}
	}
}

void TextureHandler::generateTestTextures (
		TextureInfo** texdsts,
		uint8_t numtexes) {
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			std::uniform_real_distribution unidist(0., 1.);
			glm::vec4* finaltex = (glm::vec4*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
			float** turbulentsaw = new float* [texdsts[texidx]->resolution.width];
			for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
				turbulentsaw[x] = new float[texdsts[texidx]->resolution.height];
				memset(turbulentsaw[x], 0, texdsts[texidx]->resolution.height * sizeof(float));
			}
			for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
				for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
					turbulentsaw[x][y] = -10. * abs(sin((float)x / (float)texdsts[texidx]->resolution.width)) + 1
										 + 0.5 * turbulence[x % turbulenceresolution][y % turbulenceresolution];

				}
			}
			normalizedColorMap(turbulentsaw, &finaltex, texdsts[texidx]->resolution.width, 16);
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(finaltex));
			for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) delete[] turbulentsaw[x];
			delete[] turbulentsaw;
			delete[] finaltex;
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_NORMAL || texdsts[texidx]->type == TEXTURE_TYPE_HEIGHT) {
			size_t size = texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4);
			glm::vec4* data = (glm::vec4*)malloc(size);
			memset(data, 0, size);
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		}
	}
}

void TextureHandler::generateOceanTextures (
		TextureInfo** texdsts,
		uint8_t numtexes) {
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		if (texdsts[texidx]->type == TEXTURE_TYPE_NORMAL) {
			glm::vec4* data = (glm::vec4*)malloc(
					texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4));
			for (uint32_t y = 0; y < texdsts[texidx]->resolution.height; y++) {
				for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) {
//					data[y*texdsts[texidx]->resolution.width+x]=glm::normalize(glm::vec4(
//							0.,
//							1.,
//							abs(sin((float)y/(float)texdsts[texidx]->resolution.height))+1
//					                   +0.5*turbulence[x%turbulenceresolution][y%turbulenceresolution],
//						    0.));
//					data[y*texdsts[texidx]->resolution.width+x]=glm::vec4(
//							0.,
//							1.,
//							sin((float)y/10.)*0.5+turbulence[x%turbulenceresolution][y%turbulenceresolution],
//							0.);

//					data[y*texdsts[texidx]->resolution.width+x]
//					=glm::normalize(glm::vec4(0.,
//						  1.,
//						  abs(sin(100.*((float)y/(float)texdsts[texidx]->resolution.height))+2.*turbulence[x%turbulenceresolution][y%turbulenceresolution]),
//						  0.));
				}
			}
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		}
	}
}

void TextureHandler::generateBlankTextures (
		TextureInfo** texdsts,
		uint8_t numtexes) {
	for (uint8_t texidx = 0; texidx < numtexes; texidx++) {
		uint64_t numtexels = texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height;
		size_t size = numtexels * sizeof(glm::vec4);
		glm::vec4* data = (glm::vec4*)malloc(size);
		if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			for (uint32_t i = 0; i < numtexels; i++) {
				data[i] = glm::vec4(1.f);
			}
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			memset(reinterpret_cast<void*>(data), 0, size);
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_DIFFUSE) {
			memset(reinterpret_cast<void*>(data), 0, size);
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
		}
		free(data);
	}
}

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
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(finaltex));
			for (uint32_t x = 0; x < texdsts[texidx]->resolution.width; x++) delete[] turbulentsquare[x];
			delete[] turbulentsquare;
			delete[] finaltex;
		} else if (texdsts[texidx]->type == TEXTURE_TYPE_NORMAL || texdsts[texidx]->type == TEXTURE_TYPE_HEIGHT) {
			size_t size = texdsts[texidx]->resolution.width * texdsts[texidx]->resolution.height * sizeof(glm::vec4);
			glm::vec4* data = (glm::vec4*)malloc(size);
			memset(data, 0, size);
			GraphicsHandler::VKHelperUpdateWholeTexture(texdsts[texidx], reinterpret_cast<void*>(data));
			free(data);
		}
	}
}

void TextureHandler::generateSkyboxTexture (TextureInfo* texdst) {
	//pass in sun for position info and color info diffusion
	glm::vec4* data = new glm::vec4[texdst->resolution.width * texdst->resolution.height * sizeof(glm::vec4) * 6];
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
				else if (layer == 5) directionvectortemp = glm::vec3((1. - uv.x) * 2. - 1., uv.y * 2. - 1., -1.);
				else directionvectortemp = glm::vec3(0.);
				directionvectortemp = glm::normalize(directionvectortemp);
				if (glm::length(directionvectortemp - glm::normalize(glm::vec3(0., 100., 500))) < 0.05) {
					data[layer * texdst->resolution.height * texdst->resolution.width + y * texdst->resolution.width +
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
					data[layer * texdst->resolution.height * texdst->resolution.width + y * texdst->resolution.width +
						 x] = glm::vec4((1. - directionvectortemp.y) * 0.5, (1. - directionvectortemp.y) * 0.5, 0.9,
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
	GraphicsHandler::VKHelperUpdateWholeTexture(texdst, data);
	delete[] data;
}

void TextureHandler::generateVec4MipmapData (
		uint32_t numlevels,
		uint32_t res,
		glm::vec4** data) {
	VkDeviceSize totalsize = 0u;
	for (uint32_t level = 0u; level < numlevels; level++) {
		totalsize += sizeof(glm::vec4) * pow(res / pow(2, level), 2);
	}
	glm::vec4* dataout = (glm::vec4*)malloc(totalsize), * mipdata;
	memcpy((void*)dataout, (void*)(*data), sizeof(glm::vec4) * pow(res, 2));
	uint32_t offset = pow(res, 2), mipsize, mipres, pixelsubres;
	glm::vec4 pixelsum;
	for (uint32_t level = 1u; level < numlevels; level++) {
		//maybe average previously calculated mipmap instead of original data??? may not work with other generation algorithms
		pixelsubres = pow(2, level);
		mipres = res / pixelsubres;
		mipsize = sizeof(glm::vec4) * pow(mipres, 2);
		mipdata = (glm::vec4*)malloc(mipsize);
		for (uint32_t y = 0u; y < mipres; y++) {
			for (uint32_t x = 0u; x < mipres; x++) {
//				std::cout<<"x: "<<x<<", y: "<<y<<'\n';
				pixelsum = glm::vec4(0.0);
				for (uint32_t v = 0u; v < pixelsubres; v++) {
					for (uint32_t u = 0u; u < pixelsubres; u++) {
//						pixelsum+=(*data)[(res/pixelsubres*y+x)+v*pixelsubres+u];
						pixelsum += (*data)[res * pixelsubres * y + pixelsubres * x + res * v + u];
//						std::cout<<"\tadding in ";
//						PRINT_QUAD((*data)[res*pixelsubres*y+pixelsubres*x+res*v+u])<<'\n';
						//still unsure about this...
					}
				}
				mipdata[y * mipres + x] = pixelsum / pow(pixelsubres, 2);
//				std::cout<<"avg'd to ";
//				PRINT_QUAD(mipdata[y*mipres+x])<<'\n';
			}
		}
		memcpy((void*)(dataout + offset), (void*)mipdata, mipsize);
		free(mipdata);
		offset += pow(mipres, 2);
	}
	*data = dataout;
}


float TextureHandler::linearInterpolatedNoise (glm::vec2 T, uint32_t N, float** noise) {
	//consider 2D catmull-rom splines later
	// ^^^ what does this mean lol???
	glm::ivec2 TI = glm::vec2((int)T.x, (int)T.y);
	glm::vec2 dT = T - (glm::vec2)TI;
	TI = glm::ivec2(TI.x % N, TI.y % N);
	if (T.x >= 0 && T.y >= 0) {
		float xinterpa = dT.x * noise[(TI.x + 1) % N][TI.y] + (1.0 - dT.x) * noise[TI.x][TI.y],
				xinterpb = dT.x * noise[(TI.x + 1) % N][(TI.y + 1) % N] + (1.0 - dT.x) * noise[TI.x][(TI.y + 1) % N];
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

void TextureHandler::generateNormalFromHeight (
		float** src, glm::vec4** dst, uint32_t resh, uint32_t resn, float worldspacetotexspace) {
	//worldspacetotexspace should be calculated such that multiplying it by multiplying it by a vec2 w/ normalized tex
	//coords converts them to world-space coords (scale-wise, offset is unimportant (i think???))
	//but doesn't the above vary by tri and its uvs coords???? IT DOES,,, soooooo we need to pass in the mesh, determine
	//which tri we're on, ///~~~THENNNN~~~/// calculate barycentric coords *AGAIN* to determine scaling factor, then
	//do our funky triangle math. fuck it for now, ill just calc on the fly and focus on more important stuff lol


//	float ntohconv=(float)resh/(float)resn;
//	glm::ivec2 flooredhcoords;
//	glm::vec3 normtemp;
//	for(uint32_t y=0;y<resn;y++){
//		for(uint32_t x=0;x<resn;x++){
//			flooredhcoords=glm::ivec2((int)((float)x*ntohconv), (int)((float)y*ntohconv));
//			if((float)y-(float)flooredhcoords.y<=(float)x-(float)flooredhcoords.x){
//				normtemp=glm::cross()
//			}
//			else{
//
//			}
//		}
//	}
}

glm::vec4 TextureHandler::normalizedColorRamp (float x) {
	return glm::vec4(1., 0., 0., 1.) * sin(x * 3.14)
		   + glm::vec4(0., 1., 0., 1.) * abs(sin(x * 3.14 + (3.14 / 3.)))
		   + glm::vec4(0., 0., 1., 1.) * abs(sin(x * 3.14 + (6.28 / 3.)));
}

void TextureHandler::normalizedColorMap (float** src, glm::vec4** dst, uint32_t dim, uint32_t numcols) {
	glm::vec4 colors[numcols];
	std::uniform_real_distribution unidist(0., 1.);
	for (uint32_t i = 0; i < numcols; i++) {
		colors[i] = glm::vec4(unidist(randomengine), unidist(randomengine), unidist(randomengine), 1.);
	}
	for (uint32_t u = 0; u < dim; u++) {
		for (uint32_t v = 0; v < dim; v++) {
			(*dst)[u * dim + v] = colors[int(src[u][v] * numcols) % numcols];
		}
	}
}