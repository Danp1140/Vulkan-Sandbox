//
// Created by Daniel Paavola on 6/23/21.
//

#ifndef SANDBOX_TEXT_H
#define SANDBOX_TEXT_H

#include "GraphicsHandler.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define MAX_TEXTURE_WIDTH 1024
#define MAX_TEXTURE_HEIGHT 1024

class Text {
private:
	std::string message;
	glm::vec4 fontcolor;
	uint32_t horizontalres, verticalres;
	float fontsize;
	TextPushConstants pushconstants;
	VkDescriptorSet* descriptorsets;
	TextureInfo* textures;

	void regenFaces (bool init);

	void initDescriptorSet ();

public:
	FT_Library ftlib;
	FT_Face face;

	Text ();

	Text (std::string m, glm::vec2 p, glm::vec4 mc, float fs, int hr, int vr);

	~Text ();

	void setMessage (std::string m, uint32_t index);

	static void recordCommandBuffer (cbRecData data, VkCommandBuffer& cb);

	VkDescriptorSet* getDescriptorSets () {return descriptorsets;}

	TextPushConstants* getPushConstantData () {return &pushconstants;}
};

#endif //SANDBOX_TEXT_H
