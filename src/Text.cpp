//
// Created by Daniel Paavola on 6/23/21.
//

#include "Text.h"

Text::Text () : 
	message(""), 
	fontcolor(glm::vec4(0.5, 0.5, 0.5, 1)), 
	fontsize(12),
	pushconstants({glm::vec2(0), glm::vec2(1), 0}) {
	textures = new TextureInfo[GraphicsHandler::vulkaninfo.numswapchainimages];
	ftlib = FT_Library();
	FT_Init_FreeType(&ftlib);
	FT_New_Face(ftlib, WORKING_DIRECTORY "/resources/fonts/arial.ttf", 0, &face);
	//unsure what params mean in this function...
	FT_Set_Char_Size(face, 0, fontsize * 64, 0, 72);
	//probably shouldnt call this function on a null-init'd text
	regenFaces(true);
	initDescriptorSet();
}

Text::Text (std::string m, glm::vec2 p, glm::vec4 mc, float fs, int hr, int vr) :
	message(m),
	fontcolor(mc),
	fontsize(fs),
	horizontalres(hr),
	verticalres(vr),
	pushconstants({p, glm::vec2(2.0f / (float)hr, 2.0f / (float)vr), 0}) {
	textures = new TextureInfo[MAX_FRAMES_IN_FLIGHT];
	ftlib = FT_Library();
	FT_Init_FreeType(&ftlib);
	FT_New_Face(ftlib, WORKING_DIRECTORY "/resources/fonts/arial.ttf", 0, &face);
	//unsure what params mean in this function...
	//unit of width is ???, unit of resolution is dpi
	FT_Set_Char_Size(face, 0, fontsize * 64, 0, 36);
	regenFaces(true);
	initDescriptorSet();
}

Text::~Text () {
	FT_Done_Face(face);
	FT_Done_FreeType(ftlib);
	for (uint32_t x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++) {
		vkDestroySampler(GraphicsHandler::vulkaninfo.logicaldevice, textures[x].sampler, nullptr);
		vkDestroyImageView(GraphicsHandler::vulkaninfo.logicaldevice, textures[x].imageview, nullptr);
		vkDestroyImage(GraphicsHandler::vulkaninfo.logicaldevice, textures[x].image, nullptr);
		vkFreeMemory(GraphicsHandler::vulkaninfo.logicaldevice, textures[x].memory, nullptr);
	}
	delete[] descriptorsets;
	delete[] textures;
}

void Text::createPipeline() {
	VkDescriptorSetLayoutBinding objectdslbinding {
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	};
	VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			0,
			nullptr
		}, {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			1,
			&objectdslbinding
	}};

	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pii.shaderfilepathprefix = "text";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TextPushConstants)};
	pii.renderpass = GraphicsHandler::vulkaninfo.compositingrenderpass;

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.textgraphicspipeline, pii);
}

// TODO: get rid of index lmao
void Text::setMessage (std::string m, uint32_t index) {
	// kinda feel like this func should be able to handle changing for all fif...
	// hold on a fucking second, we dont need a texture for each fif wtf is this shit
	message = m;
	pushconstants.scale = glm::vec2(2.0f / (float)horizontalres, 2.0f / (float)verticalres);
	regenFaces(false);
	/*VkDescriptorImageInfo imginfo = {
			textures[0].sampler,
			textures[0].imageview,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkWriteDescriptorSet write {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorsets[GraphicsHandler::vulkaninfo.currentframeinflight],
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imginfo,
			nullptr,
			nullptr
	};
	vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &write, 0, nullptr);
	 */
}

void Text::regenFaces (bool init) {
	unsigned int maxlinelength = 0u, linelengthcounter = 0u, numlines = 1u;
	for (char c: message) {
		if (c == '\n') {
			if (linelengthcounter > maxlinelength) maxlinelength = linelengthcounter;
			linelengthcounter = 0u;
			numlines++;
			continue;
		}
		FT_Load_Char(face, c, FT_LOAD_DEFAULT);
		linelengthcounter += face->glyph->metrics.horiAdvance;
	}
	if (linelengthcounter > maxlinelength) maxlinelength = linelengthcounter;
//	const uint32_t hres=maxlinelength/64u, vres=numlines*face->size->metrics.height/64u;
	const uint32_t hres = MAX_TEXTURE_WIDTH, vres = MAX_TEXTURE_HEIGHT;
	float* texturedata = (float*)malloc(hres * vres * sizeof(float));
	memset(&texturedata[0], 0.0f, hres * vres * sizeof(float));
	pushconstants.scale *= glm::vec2(hres, vres);
	glm::vec2 penposition = glm::ivec2(0, vres - face->size->metrics.ascender / 64);
	// i legitimately do not understand how this is working so well in real time, updating every frame. it may be taking
	// a hidden tax on performance, so still consider optimizations, as there could be some, especially if we can update
	// only part of the texture or stream the data straight from the freetype bitmap to the gpu memory
	for (char c: message) {
		if (c == '\n') {
			penposition.y -= face->size->metrics.height / 64;
			penposition.x = 0;
			continue;
		}
		FT_Load_Char(face, c, FT_LOAD_RENDER);
		if (face->glyph != nullptr) FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		penposition += glm::vec2(face->glyph->metrics.horiBearingX / 64, face->glyph->metrics.horiBearingY / 64);
		unsigned char* bitmapbuffer = face->glyph->bitmap.buffer;
		uint32_t xtex = 0, ytex = 0;
		for (uint32_t y = 0; y < face->glyph->bitmap.rows; y++) {
			for (uint32_t x = 0; x < face->glyph->bitmap.width; x++) {
				xtex = (int)penposition.x + x;
				ytex = (int)penposition.y - y;
				texturedata[ytex * hres + xtex] = *bitmapbuffer++;
			}
		}
		penposition += glm::vec2((face->glyph->metrics.horiAdvance - face->glyph->metrics.horiBearingX) / 64,
								 -face->glyph->metrics.horiBearingY / 64);
	}
	if (init) {
		for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
			textures[x].resolution = {hres, vres};
			textures[x].format = VK_FORMAT_R32_SFLOAT;
			textures[x].memoryprops = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			textures[x].type = TEXTURE_TYPE_DIFFUSE;
			textures[x].sampler = GraphicsHandler::genericsampler;
			GraphicsHandler::createTexture(textures[x]);
		}
	} else {
		//could we just use VK_FORMAT_R8_UNORM? probably would need reformatting cause UNORM
//		GraphicsHandler::VKHelperUpdateWholeVisibleTexture(
//				&textures[GraphicsHandler::vulkaninfo.currentframeinflight],
//				reinterpret_cast<void*>(texturedata));
		GraphicsHandler::VKHelperUpdateWholeTexture(
				textures[0],
				reinterpret_cast<void*>(texturedata));
	}
	delete (texturedata);
}

void Text::initDescriptorSet () {
	VkDescriptorSetLayout layoutstemp[MAX_FRAMES_IN_FLIGHT];
	for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++)
		layoutstemp[x] = GraphicsHandler::vulkaninfo.textgraphicspipeline.objectdsl;
	descriptorsets = new VkDescriptorSet[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSetAllocateInfo allocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			MAX_FRAMES_IN_FLIGHT,
			&layoutstemp[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &allocinfo, &descriptorsets[0]);
	for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		VkDescriptorImageInfo imginfo = textures[0].getDescriptorImageInfo();
		VkWriteDescriptorSet write {
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
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &write, 0, nullptr);
	}
}           //perhaps make a more general version of this as a VKHelper, cause we're doing this in a couple places

void Text::recordCommandBuffer (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			data.renderpass,
			0,
			data.framebuffer,
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
			&cmdbufinherinfo
	};
	vkBeginCommandBuffer(cb, &cmdbufbegininfo);
	vkCmdBindPipeline(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipeline);
	vkCmdPushConstants(
			cb,
			data.pipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(TextPushConstants),
			data.pushconstantdata);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipelinelayout,
			0,
			1, &data.descriptorset,
			0, nullptr);
	vkCmdDraw(cb, 6, 1, 0, 0);
	vkEndCommandBuffer(cb);
}
