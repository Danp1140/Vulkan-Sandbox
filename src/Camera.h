//
// Created by Daniel Paavola on 5/30/21.
//

#ifndef SANDBOX_CAMERA_H
#define SANDBOX_CAMERA_H

#include "GraphicsHandler.h"

enum ProjectionType {
	PERSPECTIVE
};

class Camera {
private:
	glm::vec3 position, forward, up;
	float fovy, nearclip, farclip, aspectratio, movementsens, mousesens;
	double mousex, mousey, lastmousex, lastmousey;
	ProjectionType projection;
	glm::mat4 projectionmatrix, viewmatrix;

	void recalculateProjectionMatrix ();

	void recalculateViewMatrix ();

public:
	Camera ();

	Camera (GLFWwindow* w, int hres, int vres);

	void takeInputs (GLFWwindow* w);

	void calculateFrustum (glm::vec3* dst);

	void setPosition (glm::vec3 p);

	glm::vec3 getPosition () {return position;}

	glm::vec3 getForward () {return forward;}

	glm::mat4 getProjectionMatrix () {return projectionmatrix;}

	glm::mat4 getViewMatrix () {return viewmatrix;}

	glm::mat4 calcAndGetSkyboxViewMatrix ();

	float getFovy () {return fovy;}
};


#endif //SANDBOX_CAMERA_H
