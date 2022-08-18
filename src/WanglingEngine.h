/* File: WanglingEngine.h
 * Author: Daniel Paavola
 *
 * WanglingEngine.h defines the WanglingEngine class, which should be instantiated once in main.cpp. It exists at the
 * very top of the include tree and brings together every other class and functionality, keeping track of meshes,
 * lights, and the camera, as well as dispatching physics calculations, texture generation, command buffer recording
 * threads, and the actual command buffer submission and framebuffer presentation with Vulkan. draw() is be called
 * in a loop inside main.cpp in order to allow WanglingEngine to function.
 */

#ifndef VULKANSANDBOX_WANGLINGENGINE_H
#define VULKANSANDBOX_WANGLINGENGINE_H

#include "Mesh.h"
#include "Camera.h"
#include "Light.h"
#include "Text.h"
#include "TextureHandler.h"
#include "Ocean.h"
#include "ParticleSystem.h"

#include <thread>
#include <rapidjson/document.h>
#include <queue>

#define TIME_OPERATION(op){ \
        start=std::chrono::high_resolution_clock::now(); \
        op; \
        GraphicsHandler::troubleshootingsstrm<<"\nline "<<__LINE__<<" execution time: "\
        <<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count();\
}
#define PRINT_TIME_OPERATION(op){ \
        start=std::chrono::high_resolution_clock::now(); \
        op; \
        std::cout<<"\nline "<<__LINE__<<" execution time: "\
        <<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count()<<std::endl;\
}
#define VERBOSE_TIME_OPERATION(op){ \
        start=std::chrono::high_resolution_clock::now(); \
        op; \
        GraphicsHandler::troubleshootingsstrm<<'\n'<<#op<<" execution time: "\
        <<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count(); \
}

class WanglingEngine {
private:
	Camera* primarycamera;
	std::vector<Mesh*> meshes;
	std::vector<Light*> lights;
	Ocean* ocean;
	ParticleSystem<GrassParticle>* grass;
	Text* troubleshootingtext;
	std::vector<glm::vec3> troubleshootinglines;
	PhysicsHandler physicshandler;
	TextureHandler texturehandler;
	VkCommandBuffer* skyboxcommandbuffers, * texmoncommandbuffers, * troubleshootinglinescommandbuffers, * ssrrcpycommandbuffers;
	std::thread recordingthreads[NUM_RECORDING_THREADS];
	static std::condition_variable conditionvariable;
	static std::mutex submitfencemutex;
	static bool submitfenceavailable;
	TextureInfo skyboxtexture;
	VkDescriptorSetLayout scenedsl;
	VkDescriptorSet* skyboxdescriptorsets, * texmondescriptorsets, * compositingdescriptorsets;
	static VkDescriptorSet* scenedescriptorsets;
	VkBuffer* lightuniformbuffers, troubleshootinglinesvertexbuffer = VK_NULL_HANDLE;
	VkDeviceMemory* lightuniformbuffermemories, troubleshootinglinesvertexbuffermemory;
	uint64_t multiverseseed;
	std::queue<cbRecTask> recordingtasks;
	std::queue<cbCollectInfo> secondarybuffers;
	static std::mutex recordingmutex, scenedsmutex;

	/* Below are a few initialization functions that help with one-off elements (whole-scene descriptors, skybox,
	 * troubleshooting texture monitor and line drawer).
	 */
	void initSceneData ();

	void initSkybox ();

	void initTroubleshootingLines ();

	void updateSkyboxDescriptorSets ();

//	void recordSkyboxCommandBuffers (uint8_t fifindex, uint8_t sciindex);

	static void recordSkyboxCommandBuffers (cbRecData data, VkCommandBuffer& cb);

	void recordTexMonCommandBuffers (uint8_t fifindex, uint8_t sciindex);

	void recordTroubleshootingLinesCommandBuffers (uint8_t fifindex, uint8_t sciindex, WanglingEngine* self);

	void updateTexMonDescriptorSets (TextureInfo tex);

	static void recordCommandBuffer (WanglingEngine* self, uint32_t fifindex);

	void enqueueRecordingTasks ();

	static void processRecordingTasks (
			std::queue<cbRecTask>* tasks,    // see if you can pass these by reference
			std::queue<cbCollectInfo>* resultcbs,
			uint8_t fifindex, // TODO: switch the order of fifi and threadi to match that of threadcbsets
			uint8_t threadidx,
			VkDevice device);

	// TODO: tack this onto the end of a thread
	void collectSecondaryCommandBuffers ();

	/* countSceneObjects and loadScene help with loading and pre-loading scenes from a json file. Pre-loading is done so
	 * that GraphicsHandler can know how many meshes and lights to make space for before they are actually created.
	 */
	void countSceneObjects (const char* scenefilepath, uint8_t* nummeshes, uint8_t* numlights);

	void loadScene (const char* scenefilepath);

	void genScene ();

	// TODO: read more about multi-threading/thread safety. it feels unwise and unsafe to pass in a self-reference here
	/* Significant overhaul of multi-thread recording is neccesary. Firstly, while read/write is currently /mostly/
	 * stable, it is not explicitly safe. Some useful changes/insights.
	 *      1) Obviously passing in an entire WanglingEngine* is bad practice. Instead, figure out data that needs to
	 *         be read and make a const copy of it for the threads. Then, create a multi-read lock on it. To do multi-
	 *         read, we may need to use a std::shared_mutex and a std::shared_lock.
	 *      2) To better represent recording and create a good > 1 thread implementation, try using a queue of function
	 *         pointers to represent recording work that needs to be done. Either dispatch all secondary threads from
	 *         the main loop, or dispatch one secondary that dispatches multiple tertiaries (joe's idea, not sure why
	 *         this could be better.
	 *      3) Joe also suggested using "closures," which, as he explained them, allow data to be passed between threads
	 *         I don't see a use for them here yet, but it's good to know they exist in case we need to do something
	 *         like that in the future.
	 *      4) > 1 recording thread is going to require > 1 command pool (one per thread) which will be passed to the
	 *         cmd buf begin info.
	 */
	[[noreturn]] static void threadedCmdBufRecord (WanglingEngine* self);

public:
	WanglingEngine ();

	/* draw() is –central– to how this entire program works, so it's crucial that it is clean and efficient. It is
	 * currently not. Many parts of draw should be broken into sub-functions. For instance, flag checking, cmd buf
	 * submission, swapchain presentation.
	 */
	void draw ();

	bool shouldClose ();
};


#endif //VULKANSANDBOX_WANGLINGENGINE_H
