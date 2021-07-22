//
// Created by Daniel Paavola on 6/17/21.
//

#include "Light.h"

Light::Light(){
	position=glm::vec3(0, 5, 5);
	forward=glm::vec3(0, 0, -1);
	up=glm::vec3(0, 1, 0);
	type=LIGHT_TYPE_SUN;
	shadowmapresolution=1024;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
}

Light::Light(glm::vec3 p, glm::vec3 f, int smr, LightType t){
	position=p;
	forward=f;
	up=glm::vec3(0, 1, 0);
	type=t;
	shadowmapresolution=smr;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
}

void Light::recalculateProjectionMatrix(){
	//okay so this is actually kinda complicated...
	//the LIGHT_TYPE_PLANE light will use glm::ortho, with a specific size
	//the LIGHT_TYPE_SUN light can be assumed to be orthographic, as it's distance
	//from the scene is effectively infinite (although in practice we would like it to be moveable)
	//but the LIGHT_TYPE_POINT light theoretically would use glm::perspective, but in a sphere...does that work????
	//like, fov=6.28???
	//...gonna need a cubemap for point lights...
//	projectionmatrix=glm::ortho<float>(-5, 5, -5, 5, 0, 100);
	//okay so it kinda works, but its very faint for some reason????
	//and im having the same ~everything shaded~ problem as i first did with ortho...
	if(type==LIGHT_TYPE_SUN) projectionmatrix=glm::ortho<float>(-25, 25, -25, 25, 0, 10000);
	else if(type==LIGHT_TYPE_PLANE) projectionmatrix=glm::ortho<float>(-5, 5, -5, 5, 0, 100);
	else projectionmatrix=glm::perspective<float>(0.57f, 1.0f, 1.0f, 100.0f);
//	projectionmatrix=glm::perspective<float>(0.57f, 1.0f, 1.0f, 100.0f);
}

void Light::recalculateViewMatrix(){
	viewmatrix=glm::lookAt(position, position+forward, up);
}

void Light::setPosition(glm::vec3 p){
	position=p;
	recalculateViewMatrix();
	recalculateProjectionMatrix();
}

void Light::setForward(glm::vec3 f){
	forward=f;
	recalculateViewMatrix();
	recalculateProjectionMatrix();
}