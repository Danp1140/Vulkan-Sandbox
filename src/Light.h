//
// Created by Daniel Paavola on 6/17/21.
//

#ifndef SANDBOX_LIGHT_H
#define SANDBOX_LIGHT_H

#include "GraphicsHandler.h"

typedef enum LightType{
	LIGHT_TYPE_SUN,
	LIGHT_TYPE_POINT,
	LIGHT_TYPE_PLANE,
	LIGHT_TYPE_SPOT
}LightType;

typedef enum ShadowType{
	SHADOW_TYPE_UNIFORM,
	SHADOW_TYPE_LIGHT_SPACE_PERSPECTIVE
}ShadowType;

class Light{
private:
	glm::vec3 position, forward, up, lsorthobasis[3], worldspacescenebb[2];
	glm::vec4 color;
	float intensity;
	glm::mat4 projectionmatrix, viewmatrix, lspview, lspprojection;
	uint32_t shadowmapresolution;
	LightType type;
	ShadowType shadowtype;
	TextureInfo shadowmap;
	VkRenderPass shadowrenderpass;
	VkFramebuffer shadowframebuffer;
	ShadowmapPushConstants shadowpushconstants;

	//idk what this means, i wrote it a while ago, but whatever, maybe its important
	//(but also the comment above it was completely shit for brains lol)
	/*
	 * additionally, what if we move light uniform buffers to the lights themselves, to avoid duplication
	 */

	void initRenderpassAndFramebuffer();
	void recalculateProjectionMatrix();
	void recalculateViewMatrix();
	void recalculateLSPSMMatrix(glm::vec3 cameraforward, glm::vec3 camerapos, glm::vec3*b, uint8_t numb);
public:
	Light();
	Light(glm::vec3 p, glm::vec3 f, float i, glm::vec4 c, int smr, LightType t);
	void setPosition(glm::vec3 p);
	void setForward(glm::vec3 f);
	void setWorldSpaceSceneBB(glm::vec3 min, glm::vec3 max);
	glm::mat4 getProjectionMatrix(){return projectionmatrix;}
	glm::mat4 getViewMatrix(){return viewmatrix;}
	void recalculateLSOrthoBasis(glm::vec3 cameraforward, glm::vec3 camerapos, glm::vec3*b, uint8_t numb);
	int getShadowmapResolution(){return shadowmapresolution;}
	glm::vec3 getPosition(){return position;}
	glm::vec4 getColor(){return color;}
	float getIntensity(){return intensity;}
	LightType getType(){return type;}
	ShadowType getShadowType(){return shadowtype;}
	VkRenderPass getShadowRenderPass(){return shadowrenderpass;}
	VkFramebuffer getShadowFramebuffer(){return shadowframebuffer;}
	ShadowmapPushConstants*getShadowPushConstantsPtr(){return &shadowpushconstants;}
	TextureInfo*getShadowmapPtr(){return &shadowmap;}
};


#endif //SANDBOX_LIGHT_H
