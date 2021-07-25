//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_CAMERA_H
#define SANDBOX_CAMERA_H

#include "GraphicsHandler.h"

enum ProjectionType{
	PERSPECTIVE
	//would like to add ortho later
};

class Camera{
private:
	//potentially store as position and rotation instead of position, forward, and up (cleaner but more complex)
	glm::vec3 position, forward, up;
	float fovy, nearclip, farclip, aspectratio, movementsens, mousesens;
	double mousex, mousey, lastmousex, lastmousey;
	ProjectionType projection;
	glm::mat4 projectionmatrix, viewmatrix;

	void recalculateProjectionMatrix();
	void recalculateViewMatrix();
public:
	Camera();
	Camera(GLFWwindow*w, int hres, int vres);
	void takeInputs(GLFWwindow*w);
	void setPosition(glm::vec3 p);
	glm::vec3 getPosition(){return position;}
	glm::vec3 getForward(){return forward;}
	glm::mat4 getProjectionMatrix(){return projectionmatrix;}
	glm::mat4 getViewMatrix(){return viewmatrix;}
};


#endif //SANDBOX_CAMERA_H
