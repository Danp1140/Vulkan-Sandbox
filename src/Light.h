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

class Light{
private:
	glm::vec3 position, forward, up;
	glm::vec4 color;
	float intensity;
	glm::mat4 projectionmatrix, viewmatrix;
	int shadowmapresolution;
	LightType type;

	void recalculateProjectionMatrix();
	void recalculateViewMatrix();
public:
	Light();
	Light(glm::vec3 p, glm::vec3 f, float i, glm::vec4 c, int smr, LightType t);
	void setPosition(glm::vec3 p);
	void setForward(glm::vec3 f);
	glm::mat4 getProjectionMatrix(){return projectionmatrix;}
	glm::mat4 getViewMatrix(){return viewmatrix;}
	int getShadowmapResolution(){return shadowmapresolution;}
	glm::vec3 getPosition(){return position;}
	glm::vec4 getColor(){return color;}
	float getIntensity(){return intensity;}
	LightType getType(){return type;}
};


#endif //SANDBOX_LIGHT_H
