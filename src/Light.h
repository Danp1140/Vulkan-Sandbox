//
// Created by Daniel Paavola on 6/17/21.
//

#ifndef SANDBOX_LIGHT_H
#define SANDBOX_LIGHT_H

#include "GraphicsHandler.h"

/* Struct used for Light's type member variable, which determines properties of its shadow, diffuse, lambertian, and
 * specular calculations.
 * LIGHT_TYPE_SUN is non-directional, infinitely far away in the -position direction, casting parallel rays. It does not
 * obey the inverse square law.
 * LIGHT_TYPE_POINT is non-directional, casting rays in a sphere.
 * LIGHT_TYPE_PLANE is directional, casting parallel rays.
 * LIGHT_TYPE_PLANE is directional, casting rays in a sphere limited by its fov
 * Notably, all types other than LIGHT_TYPE_SUN have yet to be fully implemented, and therefore will not work well.
 */
typedef enum LightType {
	LIGHT_TYPE_SUN,
	LIGHT_TYPE_POINT,
	LIGHT_TYPE_PLANE,
	LIGHT_TYPE_SPOT
} LightType;

typedef enum ShadowType {
	SHADOW_TYPE_UNIFORM,
	SHADOW_TYPE_CAMERA_SPACE_PERSPECTIVE,
	SHADOW_TYPE_LIGHT_SPACE_PERSPECTIVE
} ShadowType;

class Light {
private:
	glm::vec3 position, forward, up, lsorthobasis[3], worldspacescenebb[2];
	glm::vec4 color;
	float intensity;
	glm::mat4 projectionmatrix, viewmatrix, lspview, lspprojection;
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

	void initRenderpassAndFramebuffer ();

	/* recalculateProjectionMatrix, ViewMatrix, and LSPSMMatrix are all used to update the transform and projection
	 * matrices used to render the light's shadowmap. They calculate relavent matrices and place them directly into the
	 * shadowpushconstants member. While recalculateProjectionMatrix and ViewMatrix are somewhat self-explanatory, LSPSM
	 * may need some explanation. It is used to implement optional light-space projective shadow mapping, as described
	 * Wimmer et al.'s 2004 paper. Since the implementation is still under development, setting shadowtype to
	 * SHADOW_TYPE_UNIFORM will cause calculateLSPSMMatrix to simply set shadowpushconstants.lspmatrix to an identity
	 * matrix.
	 * TODO: cleanup each of these functions to absolutely maximize efficiency.
	 * these are called every frame and any inefficiency will take a serious toll.
	 */
	void recalculateProjectionMatrix ();

	void recalculateViewMatrix ();

	void recalculateLSPSMMatrix (glm::vec3 cameraforward, glm::vec3 camerapos, glm::vec3* b, uint8_t numb);

public:
	Light ();

	Light (glm::vec3 p, glm::vec3 f, float i, glm::vec4 c, uint32_t smr, LightType t);

	// TODO: consolidate gets and sets
	// TODO: update functions that pass by pointer to pass by const &
	// TODO: evaluate usefulness of updateMatrices
	void setPosition (glm::vec3 p);

	void setForward (glm::vec3 f);

	void setWorldSpaceSceneBB (glm::vec3 min, glm::vec3 max);

	glm::mat4 getProjectionMatrix () {return projectionmatrix;}

	glm::mat4 getViewMatrix () {return viewmatrix;}

	void updateMatrices (glm::vec3 cameraforward, glm::vec3 camerapos, glm::vec3* b, uint8_t numb);

	// for now assuming a square resolution to avoid annoying refactor lol
	uint32_t getShadowmapResolution () {return shadowmap.resolution.width;}

	glm::vec3 getPosition () {return position;}

	glm::vec4 getColor () {return color;}

	float getIntensity () {return intensity;}

	LightType getType () {return type;}

	ShadowType getShadowType () {return shadowtype;}

	VkRenderPass getShadowRenderPass () {return shadowrenderpass;}

	VkFramebuffer getShadowFramebuffer () {return shadowframebuffer;}

	ShadowmapPushConstants* getShadowPushConstantsPtr () {return &shadowpushconstants;}

	TextureInfo* getShadowmapPtr () {return &shadowmap;}
};

#endif //SANDBOX_LIGHT_H
