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
#include "TextureHandler.h"
#include "Ocean.h"
#include "ParticleSystem.h"

#include <thread>
#include <rapidjson/document.h>

#define NUM_RECORDING_THREADS 1
#define TIME_OPERATION(op){ \
        start=std::chrono::high_resolution_clock::now(); \
        op; \
        GraphicsHandler::troubleshootingsstrm<<"\nline "<<__LINE__<<" execution time: "\
        <<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count();\
}
#define VERBOSE_TIME_OPERATION(op){ \
        start=std::chrono::high_resolution_clock::now(); \
        op; \
        GraphicsHandler::troubleshootingsstrm<<'\n'<<#op<<" execution time: "\
        <<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count(); \
}

class WanglingEngine{
private:
	Camera*primarycamera;
	std::vector<Mesh*>meshes;
	std::vector<Light*>lights;
	Ocean*ocean;
	ParticleSystem<GrassParticle>*grass;
	Text*troubleshootingtext;
	PhysicsHandler physicshandler;
	TextureHandler texturehandler;
	VkCommandBuffer*skyboxcommandbuffers, *texmoncommandbuffers;
	std::thread recordingthreads[NUM_RECORDING_THREADS];
	static std::condition_variable conditionvariable;
	static std::mutex submitfencemutex;
	static bool submitfenceavailable;
	TextureInfo skyboxtexture;
	VkDescriptorSetLayout scenedsl;
	VkDescriptorSet*skyboxdescriptorsets, *scenedescriptorsets, *texmondescriptorsets;
	VkBuffer*lightuniformbuffers;
	VkDeviceMemory*lightuniformbuffermemories;

	void initSceneData();
	void initSkybox();
	void updateSkyboxDescriptorSets();
	void recordSkyboxCommandBuffers(uint8_t fifindex, uint8_t sciindex);
	void recordTexMonCommandBuffers(uint8_t fifindex, uint8_t sciindex);
	void updateTexMonDescriptorSets(TextureInfo tex);
	static void recordCommandBuffer(WanglingEngine*self, uint32_t fifindex, bool resetfence);
	void countSceneObjects(const char*scenefilepath, uint8_t*nummeshes, uint8_t*numlights);
	void loadScene(const char*scenefilepath);

	[[noreturn]] static void threadedCmdBufRecord(WanglingEngine*self);
public:
	WanglingEngine();
	void draw();
	bool shouldClose();
};


#endif //VULKANSANDBOX_WANGLINGENGINE_H
