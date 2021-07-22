//
// Created by Daniel Paavola on 7/10/21.
//

#ifndef VULKANSANDBOX_PHYSICSHANDLER_H
#define VULKANSANDBOX_PHYSICSHANDLER_H

#define STEP_HEIGHT 0.1f

#include "Camera.h"

typedef struct PhysicsObject{
	glm::vec3 position, velocity, acceleration;
	float mass;
}PhysicsObject;

class PhysicsHandler{
private:
	Mesh*currentland;
	Tri*standingtri;
	Camera*camera;
	PhysicsObject cameraPO;

	static bool edgeTest2D(glm::vec2 a, glm::vec2 b);
	static bool triIntersectionNoY(glm::vec3 tripoints[3], glm::vec3 q);
public:
	PhysicsHandler();
	PhysicsHandler(Mesh*cl, Camera*c);

	void updateStandingTri();
	void updateCameraPos();
	Tri*getStandingTri(){return standingtri;}
};


#endif //VULKANSANDBOX_PHYSICSHANDLER_H
