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
#include "Terrain.h"

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
#define NUM_FRAME_SAMPLES 60 // number of frames to sample for fps statistics

typedef enum SettingTypes {
	SETTING_TYPE_TOGGLE,
	SETTING_TYPE_RANGE
} SettingTypes;
typedef int SettingType;
typedef struct SettingDataToggle {
	bool on;
} SettingDataToggle;
typedef struct SettingDataRange {
	float min, max, value;
} SettingDataRange;
typedef union SettingData {
	SettingDataToggle toggle;
	SettingDataRange range;
} SettingData;
typedef struct Setting {
	const char* name;
	int hotkey;
	SettingType type;
	SettingData data;
} Setting;
typedef struct SettingsFolder {
	const char* name;
	int hotkey;
	std::vector<SettingsFolder> folders;
	std::vector<Setting> settings;
} SettingsFolder;

class WanglingEngine {
private:
	Camera* primarycamera;
	std::vector<Mesh*> meshes;
	std::vector<Light*> lights;
	Ocean* ocean;
	ParticleSystem<GrassParticle>* grass;
	Text* troubleshootingtext, * settingstext;
	std::vector<glm::vec3> troubleshootinglines;
	PhysicsHandler physicshandler;
	TextureHandler texturehandler;
	VkCommandBuffer* skyboxcommandbuffers, * troubleshootinglinescommandbuffers;
	std::thread recordingthreads[NUM_RECORDING_THREADS];
	TextureInfo skyboxtexture;
	VkDescriptorSetLayout scenedsl;
	VkDescriptorSet* skyboxdescriptorsets, * texmondescriptorsets;
	static VkDescriptorSet* scenedescriptorsets;
	VkBuffer* lightuniformbuffers, troubleshootinglinesvertexbuffer = VK_NULL_HANDLE;
	VkDeviceMemory* lightuniformbuffermemories, troubleshootinglinesvertexbuffermemory;
	uint64_t multiverseseed;
	std::queue<cbRecTask> recordingtasks;
	std::queue<cbCollectInfo> secondarybuffers;
	static std::mutex recordingmutex, scenedsmutex;
	Terrain* testterrain;
	float rendertimes[NUM_FRAME_SAMPLES], rendertimemean, rendertimesd;
	uint8_t framesamplecounter = 0u;
	bool bloom, ssrr;
	SettingsFolder mainsettingsfolder, * currentsettingsfolder;
	Setting* currentsetting;

	// void initSettings ();

	// void updateSettings ();

	/* Below are a few initialization functions that help with one-off elements (whole-scene descriptors, skybox,
	 * troubleshooting texture monitor and line drawer).
	 */
	void initSceneData ();
	void initSkybox ();
	void initTroubleshootingLines ();
	void updateSkyboxDescriptorSets ();
	static void recordSkyboxCommandBuffers (cbRecData data, VkCommandBuffer& cb);
	static void recordTexMonCommandBuffers (cbRecData data, VkCommandBuffer& cb);
	static void recordTroubleshootingLinesCommandBuffers (cbRecData data, VkCommandBuffer& cb);
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
	void updatePCsAndBuffers();

	void calcFrameStats (float sptime = 0.);

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
