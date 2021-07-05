//
// Created by Daniel Paavola on 6/23/21.
//

#include "Text.h"

int Text::horizontalres=0, Text::verticalres=0;

Text::Text(){
	message="";
	fontcolor=glm::vec4(0.5, 0.5, 0.5, 1);
	fontsize=12.0f;
	position=glm::vec2(0, 0);
//	ftlib=FT_Library();
//	FT_Init_FreeType(&ftlib);
//	FT_New_Face(ftlib, "../resources/fonts/arial.ttf", 0, &face);
//	//unsure what params mean in this function...
//	FT_Set_Char_Size(face, 0, fontsize*64, 1920, 1080);
//	//unsure of below line
//	regenFaces();
}

Text::Text(std::string m, glm::vec2 p, glm::vec4 mc, float fs, int hr, int vr){
	message=m;
	fontcolor=mc;
	fontsize=fs;
	horizontalres=hr; verticalres=vr;
	position=p;
//	ftlib=FT_Library();
//	FT_Init_FreeType(&ftlib);
//	FT_New_Face(ftlib, "../resources/fonts/arial.ttf", 0, &face);
//	//unsure what params mean in this function...
//	FT_Set_Char_Size(face, 0, fontsize*64, horizontalres, verticalres);
//	//unsure of below line
//	regenFaces();
}

void Text::regenFaces(){
//	float right=0, oldright=0, top=0, bottom=0;
//	FT_UInt index;
//	for(auto&c:message){
//		if(c=='\n'){
//			//unsure if this is right
//			bottom-=face->height;
//			if(right>oldright) oldright=right;
//			right=0;
//			continue;
//		}
//		index=FT_Get_Char_Index(face, c);
//		FT_Load_Glyph(face, index, FT_LOAD_RENDER);
//		if(face->glyph!=nullptr) FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
//		if(face->glyph->metrics.horiBearingY>top) top=face->glyph->metrics.horiBearingY;
//		if(face->glyph->metrics.height-face->glyph->metrics.horiBearingY<bottom) bottom=face->glyph->metrics.height-face->glyph->metrics.horiBearingY;
//		right+=face->glyph->metrics.horiAdvance;
//	}
//	if(right>oldright) oldright=right;
//	oldright/=64.0f; top/=64.0f; bottom/=64.0f;
//	vertices[0]=glm::vec2(0, top); vertices[1]=glm::vec2(0, bottom); vertices[2]=glm::vec2(oldright, bottom);
//	vertices[3]=glm::vec2(oldright, bottom); vertices[4]=glm::vec2(oldright, top); vertices[5]=glm::vec2(0, top);
//	for(auto&v:vertices) v=(v+position)/glm::vec2(horizontalres/2.0f, verticalres/2.0f)-glm::vec2(1.0f, 1.0f);
//	glm::vec2 penposition=glm::vec2(0, 0);
//	for(auto&c:message){
//		if(c=='\n'){
//			penposition+=glm::vec2(0, face->height/64.0f);
//			penposition.x=0;
//			continue;
//		}
//		index=FT_Get_Char_Index(face, c);
//		FT_Load_Glyph(face, index, FT_LOAD_RENDER);
//		if(face->glyph!=nullptr) FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
//		//add bearing to pen position?
////		std::cout<<"bottom: "<<bottom<<", top: "<<top<<", char height: "<<face->glyph->metrics.height/64.0f<<" char ybearing: "<<face->glyph->metrics.horiBearingY/64.0f<<std::endl;
////		std::cout<<c<<": "<<((top-bottom)-face->glyph->metrics.horiBearingY/64.0f)<<std::endl;
//		penposition+=glm::vec2(face->glyph->metrics.horiAdvance/64.0f, 0);
//	}
}

void Text::draw(GLuint shaders){
}