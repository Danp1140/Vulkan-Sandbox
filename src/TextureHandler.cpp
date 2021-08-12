//
// Created by Daniel Paavola on 7/31/21.
//

#include "TextureHandler.h"

TextureHandler::TextureHandler(){
	randomengine=std::default_random_engine();
//	lognormdist=std::lognormal_distribution<float>(1.0, 1.0);
}

//feels like i could handle some of this texture stuff, especailly spot/nspot handling in one function
void TextureHandler::generateStaticSandTextures(glm::vec4*sedimentdiffusegradient,
										  uint32_t hres,
										  uint32_t vres,
                                          TextureInfo*diffusetex,
                                          TextureInfo*normaltex){
	uint32_t horires=hres, vertres=vres;
	if(hres==0) horires=vres;
	if(vres==0) vertres=hres;
	float logtwo=log2(horires);
	bool spot=(horires==vertres)&&(logtwo==(int)logtwo);
	glm::vec4**diffusedst=reinterpret_cast<glm::vec4**>(&diffusetex->data);
	*diffusedst=(glm::vec4*)malloc(horires*vertres*sizeof(glm::vec4));
	std::lognormal_distribution lognormdist(0.5, 1.0);
	float value;
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			value=fmin(lognormdist(randomengine), 1.0);
			(*diffusedst)[y*horires+x]=sedimentdiffusegradient[1]*value+sedimentdiffusegradient[0]*(1.0-value);


//			(*diffusedst)[y*horires+x]=glm::vec4(1.0f)*(1.0f+sin(glm::distance(glm::vec2(x, y), glm::vec2((float)horires/2.0f, (float)vertres/2.0f))/100.0f))/2.0f;
//			(*diffusedst)[y*horires+x]=glm::vec4(sin((float)y/(float)vertres*100.0f*(1.0f+(float)rand()/(float)RAND_MAX/100.0f)));
//			(*diffusedst)[y*horires+x]=glm::vec4((float)rand()/(float)RAND_MAX, (float)rand()/(float)RAND_MAX,
//			                                     (float)rand()/(float)RAND_MAX, (float)rand()/(float)RAND_MAX);
//			(*diffusedst)[y*horires+x]=glm::vec4(sin(x/10.0), cos(y/10.0), sin(x/10.0+3.14), 1.0);
//			(*diffusedst)[y*horires+x]=glm::vec4((float)y/(float)vertres*sin(x/10.0), 0.0, (float)y/(float)vertres*sin(x/10.0+3.14), 1.0);
//			(*diffusedst)[y*horires+x]=glm::vec4((float)x/(float)horires, (float)y/(float)vertres, 0.0, 1.0);
		}
	}
//	generateVec4MipmapData(4, horires, diffusedst);
	uint32_t mipleveltemp=0u;
	glm::vec4*tempptr=((glm::vec4*)diffusetex->data);
//	tempptr+=horires*vertres+horires/2*vertres/2;
//	for(uint32_t level=1u;level<mipleveltemp;level++) tempptr+=(int)(horires/pow(2, level-1)*vertres/pow(2, level-1));
	GraphicsHandler::VKHelperInitTexture(diffusetex, horires/pow(2, mipleveltemp), vertres/pow(2, mipleveltemp), (void*)tempptr, VK_FORMAT_R32G32B32A32_SFLOAT, false);
	free(*diffusedst);
	glm::vec4**normaldst=(glm::vec4**)(&normaltex->data);
	*normaldst=(glm::vec4*)malloc(sizeof(glm::vec4)*horires*vertres);
	memset((void*)(*normaldst), 0, sizeof(glm::vec4)*horires*vertres);
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			//		(*normaldst)[x]=glm::vec4(0.0, (float)x/(float)(horires*vertres), 0.0, 1.0);
//			(*normaldst)[y*horires+x]=glm::vec4(sin((float)x/(float)horires*6.28*10.0), 0.0, sin((float)y/(float)vertres*6.28*10.0), 1.0);
			float radius=glm::length(glm::vec2((float)x/horires, (float)y/vertres));
			float theta=atan2((float)x/horires, (float)y/vertres);
			(*normaldst)[y*horires+x]=glm::vec4(sin(radius*6.28*10.0f)*sin(theta), 0.0, sin(radius*6.28*10.0f)*cos(theta), 0.0);
		}
	}
	GraphicsHandler::VKHelperInitTexture(normaltex, horires, vertres, (void*)(*normaldst), VK_FORMAT_R32G32B32A32_SFLOAT, false);
	free(*normaldst);
}

void TextureHandler::generateMonotoneTextures(
		glm::vec4 color,
		uint32_t hres,
		uint32_t vres,
		TextureInfo*diffusetex,
		TextureInfo*normaltex){
	uint32_t horires=hres, vertres=vres;
	if(hres==0) horires=vres;
	if(vres==0) vertres=hres;
	float logtwo=log2(horires);
	bool spot=(horires==vertres)&&(logtwo==(int)logtwo);
	glm::vec4**diffusedst=reinterpret_cast<glm::vec4**>(&diffusetex->data);
	*diffusedst=(glm::vec4*)malloc(horires*vertres*sizeof(glm::vec4));
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			(*diffusedst)[y*horires+x]=color;
		}
	}
	GraphicsHandler::VKHelperInitTexture(diffusetex, horires, vertres, diffusetex->data, VK_FORMAT_R32G32B32A32_SFLOAT, false);
	free(*diffusedst);
	glm::vec4**normaldst=(glm::vec4**)(&normaltex->data);
	*normaldst=(glm::vec4*)malloc(sizeof(glm::vec4)*horires*vertres);
	memset((void*)(*normaldst), 0, sizeof(glm::vec4)*horires*vertres);
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			(*normaldst)[y*horires+x]=glm::vec4(0.5*sin(x/100.0), 0.0, 0.5*sin(y/100.0), 1.0);
		}
	}
	GraphicsHandler::VKHelperInitTexture(normaltex, horires, vertres, (void*)(*normaldst), VK_FORMAT_R32G32B32A32_SFLOAT, false);
	free(*normaldst);
}

void TextureHandler::generateOceanTextures(
		uint32_t hres,
		uint32_t vres,
		TextureInfo*heighttex,
		TextureInfo*normaltex){
	uint32_t horires=hres, vertres=vres;
	if(hres==0) horires=vres;
	if(vres==0) vertres=hres;
	float logtwo=log2(horires);
	bool spot=(horires==vertres)&&(logtwo==(int)logtwo);

	float**heightdst=reinterpret_cast<float**>(&heighttex->data);
	*heightdst=(float*)malloc(horires*vertres*sizeof(float));
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			(*heightdst)[y*horires+x]=sin((x+y)/25.0)*0.25;
		}
	}
	GraphicsHandler::VKHelperInitTexture(heighttex, horires, vertres, (void*)(*heightdst), VK_FORMAT_R32_SFLOAT, false);
	free(*heightdst);

	glm::vec4**normaldst=reinterpret_cast<glm::vec4**>(&normaltex->data);
	*normaldst=(glm::vec4*)malloc(horires*vertres*sizeof(glm::vec4));
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			(*normaldst)[y*horires+x]=glm::vec4(0.0, 0.0, 0.0, 0.0);
		}
	}
	GraphicsHandler::VKHelperInitTexture(normaltex, horires, vertres, (void*)(*normaldst), VK_FORMAT_R32G32B32A32_SFLOAT, false);
	free(*normaldst);
}

void TextureHandler::updateOceanTextures(
											uint32_t hres,
											uint32_t vres,
											TextureInfo*heighttex,
											TextureInfo*normaltex){
	uint32_t horires=hres, vertres=vres;
	if(hres==0) horires=vres;
	if(vres==0) vertres=hres;
	float logtwo=log2(horires);
	bool spot=(horires==vertres)&&(logtwo==(int)logtwo);

	float offset=glfwGetTime();
	float**heightdst=reinterpret_cast<float**>(&heighttex->data);
	*heightdst=(float*)malloc(horires*vertres*sizeof(float));
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			(*heightdst)[y*horires+x]=sin((x+y+offset)/25.0)*0.25;
		}
	}
//	GraphicsHandler::VKHelperInitTexture(heighttex, horires, vertres, (void*)(*heightdst), VK_FORMAT_R32_SFLOAT, false);
	GraphicsHandler::VKHelperUpdateTexture(heighttex, 0, {0, 0}, {horires, vertres}, (void*)(*heightdst), VK_FORMAT_R32_SFLOAT);
	free(*heightdst);

	glm::vec4**normaldst=reinterpret_cast<glm::vec4**>(&normaltex->data);
	*normaldst=(glm::vec4*)malloc(horires*vertres*sizeof(glm::vec4));
	for(uint32_t y=0;y<vertres;y++){
		for(uint32_t x=0;x<horires;x++){
			(*normaldst)[y*horires+x]=glm::vec4(0.0, 0.0, 0.0, 0.0);
		}
	}
//	GraphicsHandler::VKHelperInitTexture(normaltex, horires, vertres, (void*)(*normaldst), VK_FORMAT_R32G32B32A32_SFLOAT, false);
	GraphicsHandler::VKHelperUpdateTexture(normaltex, 0, {0, 0}, {horires, vertres}, (void*)(*normaldst), VK_FORMAT_R32G32B32A32_SFLOAT);
	free(*normaldst);
}

void TextureHandler::generateVec4MipmapData(uint32_t numlevels,
	                               uint32_t res,
	                               glm::vec4**data){
	VkDeviceSize totalsize=0u;
	for(uint32_t level=0u;level<numlevels;level++){
		totalsize+=sizeof(glm::vec4)*pow(res/pow(2, level), 2);
	}
	glm::vec4*dataout=(glm::vec4*)malloc(totalsize), *mipdata;
	memcpy((void*)dataout, (void*)(*data), sizeof(glm::vec4)*pow(res, 2));
	uint32_t offset=pow(res, 2), mipsize, mipres, pixelsubres;
	glm::vec4 pixelsum;
	for(uint32_t level=1u;level<numlevels;level++){
		//maybe average previously calculated mipmap instead of original data??? may not work with other generation algorithms
		pixelsubres=pow(2, level);
		mipres=res/pixelsubres;
		mipsize=sizeof(glm::vec4)*pow(mipres, 2);
		mipdata=(glm::vec4*)malloc(mipsize);
		for(uint32_t y=0u;y<mipres;y++){
			for(uint32_t x=0u;x<mipres;x++){
//				std::cout<<"x: "<<x<<", y: "<<y<<'\n';
				pixelsum=glm::vec4(0.0);
				for(uint32_t v=0u;v<pixelsubres;v++){
					for(uint32_t u=0u;u<pixelsubres;u++){
//						pixelsum+=(*data)[(res/pixelsubres*y+x)+v*pixelsubres+u];
						pixelsum+=(*data)[res*pixelsubres*y+pixelsubres*x+res*v+u];
//						std::cout<<"\tadding in ";
//						PRINT_QUAD((*data)[res*pixelsubres*y+pixelsubres*x+res*v+u])<<'\n';
						//still unsure about this...
					}
				}
				mipdata[y*mipres+x]=pixelsum/pow(pixelsubres, 2);
//				std::cout<<"avg'd to ";
//				PRINT_QUAD(mipdata[y*mipres+x])<<'\n';
			}
		}
		memcpy((void*)(dataout+offset), (void*)mipdata, mipsize);
		free(mipdata);
		offset+=pow(mipres, 2);
	}
	*data=dataout;
}