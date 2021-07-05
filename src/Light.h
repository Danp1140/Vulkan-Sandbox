//
// Created by Daniel Paavola on 6/17/21.
//

#ifndef SANDBOX_LIGHT_H
#define SANDBOX_LIGHT_H

#include "Mesh.h"

#define MAX_LIGHTS 4

enum LightType{
	SUN,
	POINT,
	PLANE,
	SPOT
};

class Light{
private:
	glm::vec3 position, forward, up;
	glm::mat4 projectionmatrix, viewmatrix;
	int shadowmapresolution;
	LightType type;

	void recalculateProjectionMatrix();
	void recalculateViewMatrix();
public:
	Light();
	Light(glm::vec3 p, glm::vec3 f, int smr, LightType t);
	void setPosition(glm::vec3 p);
	void setForward(glm::vec3 f);
	glm::mat4 getProjectionMatrix(){return projectionmatrix;}
	glm::mat4 getViewMatrix(){return viewmatrix;}
	int getShadowmapResolution(){return shadowmapresolution;}
	glm::vec3 getPosition(){return position;}
	LightType getType(){return type;}
};


#endif //SANDBOX_LIGHT_H
