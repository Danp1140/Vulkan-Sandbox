//
// Created by Daniel Paavola on 6/23/21.
//

#ifndef SANDBOX_TEXT_H
#define SANDBOX_TEXT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Mesh.h"

typedef struct TextPushConstants{
	glm::vec2 position, scale;
	float rotation;
}TextPushConstants;

class Text{
private:
	std::string message;
	glm::vec4 fontcolor;
	//some efficiency possible here with element indexing, but rn im just doing quick&dirty
	static int horizontalres, verticalres;
	float fontsize;
	TextPushConstants pushconstants;
	VkDescriptorSet*descriptorsets;
	TextureInfo texture;

	void regenFaces();
	void initDescriptorSet();
public:
	FT_Library ftlib;
	FT_Face face;

	Text();
	Text(std::string m, glm::vec2 p, glm::vec4 mc, float fs, int hr, int vr);
	~Text();
	void draw(GLuint shaders);
	VkDescriptorSet*getDescriptorSetsPtr(){return descriptorsets;}
	TextPushConstants*getPushConstantsPtr(){return &pushconstants;}
};


#endif //SANDBOX_TEXT_H
