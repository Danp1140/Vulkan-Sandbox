//
// Created by Daniel Paavola on 7/10/21.
//

#ifndef VULKANSANDBOX_PHYSICSHANDLER_H
#define VULKANSANDBOX_PHYSICSHANDLER_H

#include "Mesh.h"
#include "Camera.h"
#include "GraphicsHandler.h"

#define STEP_HEIGHT 1.0f
#define PHYSICS_HANDLER_GRAVITY -9.8f

//#include "Camera.h"

typedef struct PhysicsObject{
	//store an array of forces???
	//store ptr to object?
	//store collision type???
	glm::vec3 position, velocity, acceleration, locomotionacceleration;
	double mass;
}PhysicsObject;

class PhysicsHandler{
private:
	Mesh*currentland;
	Tri*standingtri;
	glm::vec2 standinguv;
	Camera*camera;      //could consider just passing position and maybe forward
	PhysicsObject cameraPO;
	float dt;
	std::chrono::time_point<std::chrono::high_resolution_clock> lasttime;

	static bool edgeTest2D(glm::vec2 a, glm::vec2 b);
	//could streamline this w/ input mask of zeroed out normals
	static bool triIntersectionNoY(glm::vec3 tripoints[3], glm::vec3 q);
	//could also pass input mask of zeroed normals for efficiency
	static bool cylinderTriIntersection(Tri tri, glm::vec3 cylinderorigin, float r, float h);
	void updateStandingTri();
	void updateCameraPos();
public:
	PhysicsHandler();
	PhysicsHandler(Mesh*cl, Camera*c);

	void update();
	Tri*getStandingTri(){return standingtri;}
	glm::vec2 getStandingUV(){return standinguv;}
	//this is p bad practice lol
	float*getDtPtr(){return&dt;}
	glm::vec3 getWindVelocity(glm::vec3 p);
};


#endif //VULKANSANDBOX_PHYSICSHANDLER_H
