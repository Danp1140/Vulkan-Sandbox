//
// Created by Daniel Paavola on 7/22/21.
//

#ifndef VULKANSANDBOX_WANGLINGENGINE_H
#define VULKANSANDBOX_WANGLINGENGINE_H

#include "Mesh.h"
#include "Camera.h"
#include "Light.h"
#include "Text.h"
#include "PhysicsHandler.h"

class WanglingEngine{
private:
	Camera*primarycamera;
	std::vector<Mesh*>dynamicshadowcastingmeshes, landscapemeshes;
	std::vector<const Mesh*>staticshadowcastingmeshes;
	std::vector<Light*>lights;
	Text*troubleshootingtext;
	PhysicsHandler physicshandler;
	std::chrono::time_point<std::chrono::high_resolution_clock> lasttime;

	void recordCommandBuffer(uint32_t index);
public:
	WanglingEngine();
	void draw();
	bool shouldClose();
};


#endif //VULKANSANDBOX_WANGLINGENGINE_H
