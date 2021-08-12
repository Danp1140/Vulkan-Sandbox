//
// Created by Daniel Paavola on 6/23/21.
//

#include "Text.h"

int Text::horizontalres=0, Text::verticalres=0;

Text::Text(){
	message="";
	fontcolor=glm::vec4(0.5, 0.5, 0.5, 1);
	fontsize=12.0f;
	pushconstants.position=glm::vec2(0, 0);
	pushconstants.scale=glm::vec2(1.0f, 1.0f);
	pushconstants.rotation=0.0f;
	textures=new TextureInfo[GraphicsHandler::vulkaninfo.numswapchainimages];
	ftlib=FT_Library();
	FT_Init_FreeType(&ftlib);
	FT_New_Face(ftlib, "../resources/fonts/arial.ttf", 0, &face);
	//unsure what params mean in this function...
	FT_Set_Char_Size(face, 0, fontsize*64, 0, 72);
	//probably shouldnt call this function on a null-init'd text
	regenFaces(true);
	initDescriptorSet();
}

Text::Text(std::string m, glm::vec2 p, glm::vec4 mc, float fs, int hr, int vr){
	message=m;
	fontcolor=mc;
	fontsize=fs;
	horizontalres=hr; verticalres=vr;
	pushconstants.position=p;
	pushconstants.scale=glm::vec2(2.0f/(float)hr, 2.0f/(float)vr);
	pushconstants.rotation=0.0f;
	textures=new TextureInfo[GraphicsHandler::vulkaninfo.numswapchainimages];
	ftlib=FT_Library();
	FT_Init_FreeType(&ftlib);
	FT_New_Face(ftlib, "../resources/fonts/arial.ttf", 0, &face);
	//unsure what params mean in this function...
	//unit of width is ???, unit of resolution is dpi
	FT_Set_Char_Size(face, 0, fontsize*64, 0, 36);
	regenFaces(true);
	initDescriptorSet();
}

Text::~Text(){
	FT_Done_Face(face);
	FT_Done_FreeType(ftlib);
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++){
		vkDestroySampler(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, textures[x].sampler, nullptr);
		vkDestroyImageView(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, textures[x].imageview, nullptr);
		vkDestroyImage(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, textures[x].image, nullptr);
		vkFreeMemory(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, textures[x].memory, nullptr);
	}
	delete[] descriptorsets;
	delete[] textures;
}

void Text::setMessage(std::string m, uint32_t index){
	message=m;
	pushconstants.scale=glm::vec2(2.0f/(float)horizontalres, 2.0f/(float)verticalres);
	regenFaces(false);
	VkDescriptorImageInfo imginfo={
		textures[GraphicsHandler::swapchainimageindex].sampler,
		textures[GraphicsHandler::swapchainimageindex].imageview,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkWriteDescriptorSet write{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorsets[GraphicsHandler::swapchainimageindex],
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imginfo,
			nullptr,
			nullptr
	};
	vkUpdateDescriptorSets(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, 1, &write, 0, nullptr);
}

void Text::regenFaces(bool init){
	unsigned int maxlinelength=0u, linelengthcounter=0u, numlines=1u;
	for(char c:message){
		if(c=='\n'){
			if(linelengthcounter>maxlinelength) maxlinelength=linelengthcounter;
			linelengthcounter=0u;
			numlines++;
			continue;
		}
		FT_Load_Char(face, c, FT_LOAD_DEFAULT);
		linelengthcounter+=face->glyph->metrics.horiAdvance;
	}
	if(linelengthcounter>maxlinelength) maxlinelength=linelengthcounter;
	const uint32_t hres=maxlinelength/64u, vres=numlines*face->size->metrics.height/64u;
	float*texturedata=(float*)malloc(hres*vres*sizeof(float));
	memset(&texturedata[0], 0.0f, hres*vres*sizeof(float));
	pushconstants.scale*=glm::vec2(hres, vres);
	glm::vec2 penposition=glm::ivec2(0, vres-face->size->metrics.ascender/64);
	for(char c:message){
		if(c=='\n'){
			penposition.y-=face->size->metrics.height/64;
			penposition.x=0;
			continue;
		}
		FT_Load_Char(face, c, FT_LOAD_RENDER);
		if(face->glyph!=nullptr) FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		penposition+=glm::vec2(face->glyph->metrics.horiBearingX/64, face->glyph->metrics.horiBearingY/64);
		unsigned char*bitmapbuffer=face->glyph->bitmap.buffer;
		uint32_t xtex=0, ytex=0;
		for(uint32_t y=0;y<face->glyph->bitmap.rows;y++){
			for(uint32_t x=0;x<face->glyph->bitmap.width;x++){
				xtex=(int)penposition.x+x;
				ytex=(int)penposition.y-y;
				texturedata[ytex*hres+xtex]=*bitmapbuffer++;
			}
		}
		penposition+=glm::vec2((face->glyph->metrics.horiAdvance-face->glyph->metrics.horiBearingX)/64, -face->glyph->metrics.horiBearingY/64);
	}
	//if we wanna differentiate between a startup init and on-the-fly update, we gotta do some indexing of the textures below
//	GraphicsHandler::VKHelperInitTexture(&textures[GraphicsHandler::swapchainimageindex],
//	                                     hres,
//	                                     vres,
//	                                     reinterpret_cast<void*>(&texturedata[0]),
//	                                     VK_FORMAT_R32_SFLOAT,
//	                                     textures[GraphicsHandler::swapchainimageindex].image!=VK_NULL_HANDLE);
	if(init){
		for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++){
			GraphicsHandler::VKHelperInitTexture(&textures[x],
			                                     hres,
			                                     vres,
			                                     reinterpret_cast<void*>(&texturedata[0]),
			                                     VK_FORMAT_R32_SFLOAT,
			                                     false);
		}
	}
	else{
		GraphicsHandler::VKHelperInitTexture(&textures[GraphicsHandler::swapchainimageindex],
		                                     hres,
		                                     vres,
		                                     reinterpret_cast<void*>(&texturedata[0]),
		                                     VK_FORMAT_R32_SFLOAT,
		                                     true);
	}
	delete(texturedata);
}

void Text::initDescriptorSet(){
	VkDescriptorSetLayout layoutstemp[GraphicsHandler::getVulkanInfoPtr()->numswapchainimages];
	for(uint32_t x=0;x<GraphicsHandler::getVulkanInfoPtr()->numswapchainimages;x++) layoutstemp[x]=GraphicsHandler::getVulkanInfoPtr()->textgraphicspipeline.objectdsl;
	descriptorsets=new VkDescriptorSet[GraphicsHandler::getVulkanInfoPtr()->numswapchainimages];
	VkDescriptorSetAllocateInfo allocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::getVulkanInfoPtr()->descriptorpool,
		GraphicsHandler::getVulkanInfoPtr()->numswapchainimages,
		&layoutstemp[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, &allocinfo, &descriptorsets[0]);
	for(uint32_t x=0;x<GraphicsHandler::getVulkanInfoPtr()->numswapchainimages;x++){
		VkDescriptorImageInfo imginfo=textures[x].getDescriptorImageInfo();
		VkWriteDescriptorSet write{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorsets[x],
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imginfo,
			nullptr,
			nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::getVulkanInfoPtr()->logicaldevice, 1, &write, 0, nullptr);
	}
}           //perhaps make a more general version of this as a VKHelper, cause we're doing this in a couple places