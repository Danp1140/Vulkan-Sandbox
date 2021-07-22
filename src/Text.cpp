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
	ftlib=FT_Library();
	FT_Init_FreeType(&ftlib);
	FT_New_Face(ftlib, "../resources/fonts/arial.ttf", 0, &face);
	//unsure what params mean in this function...
	FT_Set_Char_Size(face, 0, fontsize*64, 0, 72);
	//probably shouldnt call this function on a null-init'd text
	regenFaces();
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
	ftlib=FT_Library();
	FT_Init_FreeType(&ftlib);
	FT_New_Face(ftlib, "../resources/fonts/arial.ttf", 0, &face);
	//unsure what params mean in this function...
	//unit of width is ???, unit of resolution is dpi
	FT_Set_Char_Size(face, 0, fontsize*64, 0, 72);
	regenFaces();
	initDescriptorSet();
}

Text::~Text(){
	FT_Done_Face(face);
	FT_Done_FreeType(ftlib);
	vkDestroySampler(Mesh::getVulkanInfoPtr()->logicaldevice, texture.sampler, nullptr);
	vkDestroyImageView(Mesh::getVulkanInfoPtr()->logicaldevice, texture.imageview, nullptr);
	vkDestroyImage(Mesh::getVulkanInfoPtr()->logicaldevice, texture.image, nullptr);
	vkFreeMemory(Mesh::getVulkanInfoPtr()->logicaldevice, texture.memory, nullptr);
	delete[] descriptorsets;
}

void Text::regenFaces(){
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
	//remember stack vs heap
	float**texturedata=new float*[hres];
	for(uint32_t x=0;x<hres;x++) texturedata[x]=new float[vres];
	for(uint32_t x=0;x<hres;x++){
		for(uint32_t y=0;y<vres;y++){
//			texturedata[x][y]=(float)y/(float)vres;
//			texturedata[x][y]=(float)x/(float)hres;
//			texturedata[x][y]=(float)(x*vres+y)/(float)()
		}
	}
	pushconstants.scale*=glm::vec2(hres, vres);
	std::cout<<"texture size: "<<hres<<'x'<<vres<<std::endl;
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
//		//try using memcpy on buffer???
		unsigned char*bitmapbuffer=face->glyph->bitmap.buffer;
		std::cout<<"bitmap size: "<<face->glyph->bitmap.width<<"x"<<face->glyph->bitmap.rows<<std::endl;
		for(uint32_t x=0;x<face->glyph->bitmap.width;x++){
			for(uint32_t y=0;y<face->glyph->bitmap.rows;y++){
//				texturedata[(int)penposition.x+x][(int)penposition.y-y]=*bitmapbuffer++;
//				texturedata[(int)penposition.x+x][(int)penposition.y-y]=bitmapbuffer[face->glyph->bitmap.rows*x+y];
			}
		}
		penposition+=glm::vec2((face->glyph->metrics.horiAdvance-face->glyph->metrics.horiBearingX)/64, -face->glyph->metrics.horiBearingY/64);
	}
	Mesh::addTexture(&texture,
	                 hres,
	                 vres,
	                 (void*)(&texturedata[0][0]),
	                 sizeof(float),
	                 VK_FORMAT_R32_SFLOAT);
	for(uint32_t x=0;x<hres;x++) delete[] texturedata[x];
	delete[] texturedata;
}

void Text::initDescriptorSet(){
	VkDescriptorSetLayout layoutstemp[Mesh::getVulkanInfoPtr()->numswapchainimages];
	for(uint32_t x=0;x<Mesh::getVulkanInfoPtr()->numswapchainimages;x++) layoutstemp[x]=Mesh::getVulkanInfoPtr()->textdescriptorsetlayout;
	descriptorsets=new VkDescriptorSet[Mesh::getVulkanInfoPtr()->numswapchainimages];
	VkDescriptorSetAllocateInfo allocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		Mesh::getVulkanInfoPtr()->descriptorpool,
		Mesh::getVulkanInfoPtr()->numswapchainimages,
		&layoutstemp[0]
	};
	vkAllocateDescriptorSets(Mesh::getVulkanInfoPtr()->logicaldevice, &allocinfo, &descriptorsets[0]);
	for(uint32_t x=0;x<Mesh::getVulkanInfoPtr()->numswapchainimages;x++){
		VkDescriptorImageInfo imginfo=texture.getDescriptorImageInfo();
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
		vkUpdateDescriptorSets(Mesh::getVulkanInfoPtr()->logicaldevice, 1, &write, 0, nullptr);
	}
}

void Text::draw(GLuint shaders){
}