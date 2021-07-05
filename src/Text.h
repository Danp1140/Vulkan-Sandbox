//
// Created by Daniel Paavola on 6/23/21.
//

#ifndef SANDBOX_TEXT_H
#define SANDBOX_TEXT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Mesh.h"

class Text{
private:
	std::string message;
	glm::vec4 fontcolor;
	//some efficiency possible here with element indexing, but rn im just doing quick&dirty
	static int horizontalres, verticalres;
	float fontsize;
//	static constexpr glm::vec2 uvs[6]={glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(1, 0),
//	                                   glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)};
	static constexpr glm::vec2 uvs[6]={glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(1, 1),
	                                   glm::vec2(1, 1), glm::vec2(1, 0), glm::vec2(0, 0)};
	glm::vec2 position, vertices[6];

	void regenFaces();
public:
	FT_Library ftlib;
	FT_Face face;

	Text();
	Text(std::string m, glm::vec2 p, glm::vec4 mc, float fs, int hr, int vr);
	void draw(GLuint shaders);
};


#endif //SANDBOX_TEXT_H
