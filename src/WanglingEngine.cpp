//
// Created by Daniel Paavola on 7/22/21.
//

#include "WanglingEngine.h"

std::mutex WanglingEngine::recordingmutex = std::mutex();
std::mutex WanglingEngine::scenedsmutex = std::mutex();
VkDescriptorSet* WanglingEngine::scenedescriptorsets = nullptr;

WanglingEngine::WanglingEngine () {
	bloom = false;
	Text::createPipeline();
	Terrain::createComputePipeline();
	Terrain::createGraphicsPipeline();
	ParticleSystem<GrassParticle>::createPipeline();
	loadScene(WORKING_DIRECTORY "/resources/scenelayouts/rocktestlayout.json");
	testterrain = new Terrain();
	testterrain->getGenPushConstantsPtr()->camerapos = primarycamera->getPosition() + glm::vec3(0., 0.5, 0.);
	physicshandler = PhysicsHandler(primarycamera);

	troubleshootingtext = new Text(
			"troubleshooting text",
			glm::vec2(-1.0f, -1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			72.0f,
			GraphicsHandler::vulkaninfo.horizontalres,
			GraphicsHandler::vulkaninfo.verticalres);

	initSkybox();

	skyboxtexture.resolution = {128, 128};
	skyboxtexture.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	skyboxtexture.type = TEXTURE_TYPE_CUBEMAP;
	skyboxtexture.sampler = GraphicsHandler::linearminmagsampler;
	skyboxtexture.viewtype = VK_IMAGE_VIEW_TYPE_CUBE;
	GraphicsHandler::createTexture(skyboxtexture);
	texturehandler.generateSkyboxTexture(&skyboxtexture);
	updateSkyboxDescriptorSets();

/*	meshes.push_back(new Mesh(WORKING_DIRECTORY "/resources/objs/smoothhipolysuzanne.obj",
							  glm::vec3(0., -1., 5.),
							  glm::vec3(1.),
							  glm::quat(sin(1.57), 0., 0., cos(1.57)), 2048u));
	TextureHandler::generateTextures({*meshes.back()->getDiffuseTexturePtr()}, TextureHandler::colorfulMarbleTexGenSet);
	*/

	initSceneData();

	// texmon.updateDescriptorSet(CompositingOp::getScratchDepthDII());
	// texmon.updateDescriptorSet(meshes.back()->getDiffuseTexturePtr()->getDescriptorImageInfo());
	texmon.updateDescriptorSet(meshes.back()->getNormalTexturePtr()->getDescriptorImageInfo());

	// TODO: move to shadowsamplerinit func
	for (uint8_t fifi = 0; fifi < MAX_FRAMES_IN_FLIGHT; fifi++) {
		VkWriteDescriptorSet shadowsamplerwrites[lights.size()];
		VkDescriptorImageInfo imginfo;
		for (uint8_t m = 0; m < meshes.size(); m++) {
			for (uint8_t l = 0; l < lights.size(); l++) {
				imginfo = lights[l]->getShadowmapPtr()->getDescriptorImageInfo();
				shadowsamplerwrites[l] = {
						VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					nullptr,
					meshes[m]->getDescriptorSet(),
					4,
					l,
					1,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					&imginfo,
					nullptr,
					nullptr
				};
			}
			vkUpdateDescriptorSets(
				GraphicsHandler::vulkaninfo.logicaldevice,
				lights.size(), &shadowsamplerwrites[0],
				0, nullptr);
		}
	}

	for (uint8_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		enqueueRecordingTasks();
		for (uint8_t i = 0; i < NUM_RECORDING_THREADS; i++) {
			recordingthreads[i] = std::thread(
				processRecordingTasks,
				&recordingtasks,
				&secondarybuffers,
				x,
				i,
				GraphicsHandler::vulkaninfo.logicaldevice);
		}
		for (uint8_t i = 0; i < NUM_RECORDING_THREADS; i++) recordingthreads[i].join();
		collectSecondaryCommandBuffers();
		GraphicsHandler::vulkaninfo.currentframeinflight++;
	}

	// TODO: move to lightuniformbufferinit func
	GraphicsHandler::vulkaninfo.currentframeinflight = 0;
	for (GraphicsHandler::swapchainimageindex = 0; GraphicsHandler::swapchainimageindex <
												   GraphicsHandler::vulkaninfo.numswapchainimages; GraphicsHandler::swapchainimageindex++) {
		LightUniformBuffer lightuniformbuffertemp[MAX_LIGHTS];
		for (uint32_t x = 0; x < lights.size(); x++) {
			lightuniformbuffertemp[x] = {
					lights[x]->getShadowPushConstantsPtr()->lightvpmatrices,
					lights[x]->getShadowPushConstantsPtr()->lspmatrix,
					lights[x]->getPosition(),
					lights[x]->getIntensity(),
					lights[x]->getForward(),
					lights[x]->getType(),
					lights[x]->getColor()
			};
		}
		GraphicsHandler::VKHelperUpdateUniformBuffer(
				MAX_LIGHTS,
				sizeof(LightUniformBuffer),
				lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
				&lightuniformbuffertemp[0]);
		for (auto& m : meshes) m->pcdata.numlights = lights.size();
		ocean->getPushConstantsPtr()->numlights = lights.size();
	}
	GraphicsHandler::swapchainimageindex = 0;

	// TextureHandler::generateTextures({*meshes[0]->getDiffuseTexturePtr()}, TextureHandler::colorfulMarbleTexGenSet);
}

WanglingEngine::~WanglingEngine () {
	vkQueueWaitIdle(GraphicsHandler::vulkaninfo.graphicsqueue); 
}

void WanglingEngine::staticInits () {
	// TODO: figure out best value for below arg
	GraphicsHandler::VKInit(200);
	GraphicsHandler::VKInitPipelines();
	TextureHandler::init();
	CompositingOp::init();
	TextureMonitor::init();
	Lines::init();
	Ocean::init();
}

void WanglingEngine::staticTerminates () {
	Ocean::terminate();
	Lines::terminate();
	TextureMonitor::terminate();
}

void WanglingEngine::countSceneObjects (const char* scenefilepath, uint8_t* nummeshes, uint8_t* numlights) {
	std::ifstream jsonfile(scenefilepath);
	std::stringstream jsonstream;
	jsonstream << jsonfile.rdbuf();
	char jsonchars[jsonstream.str().size()];
	strcpy(jsonchars, jsonstream.str().c_str());
	rapidjson::Document jsondoc;
	jsondoc.ParseInsitu(jsonchars);

	*nummeshes = jsondoc["meshes"].Size();
	*numlights = jsondoc["lights"].Size();
}

void WanglingEngine::loadScene (const char* scenefilepath) {
	std::ifstream jsonfile(scenefilepath);
	std::stringstream jsonstream;
	jsonstream << jsonfile.rdbuf();
	char jsonchars[jsonstream.str().size()];
	strcpy(jsonchars, jsonstream.str().c_str());
	rapidjson::Document jsondoc;
	jsondoc.ParseInsitu(jsonchars);

	multiverseseed = jsondoc["multiverseseed"].GetUint64();
	primarycamera = new Camera(
			GraphicsHandler::vulkaninfo.window,
			GraphicsHandler::vulkaninfo.horizontalres,
			GraphicsHandler::vulkaninfo.verticalres);
	lights = std::vector<Light*>();
	const rapidjson::Value& jsonlights = jsondoc["lights"];
	glm::vec3 positiontemp, forwardtemp;
	float intensitytemp;
	glm::vec4 colortemp = glm::vec4(0., 0., 0., 1.);
	int smrtemp;
	LightType lighttypetemp;
	for (rapidjson::SizeType i = 0; i < jsonlights.Size(); i++) {
		sscanf(jsonlights[i]["position"].GetString(), "%f %f %f", &positiontemp[0], &positiontemp[1], &positiontemp[2]);
		sscanf(jsonlights[i]["forward"].GetString(), "%f %f %f", &forwardtemp[0], &forwardtemp[1], &forwardtemp[2]);
		intensitytemp = jsonlights[i]["intensity"].GetFloat();
		sscanf(jsonlights[i]["color"].GetString(), "%f %f %f", &colortemp[0], &colortemp[1], &colortemp[2]);
		smrtemp = jsonlights[i]["shadowmapresolution"].GetInt();
		if (strcmp(jsonlights[i]["type"].GetString(), "sun") == 0) lighttypetemp = LIGHT_TYPE_SUN;
		if (strcmp(jsonlights[i]["type"].GetString(), "point") == 0) lighttypetemp = LIGHT_TYPE_POINT;
		if (strcmp(jsonlights[i]["type"].GetString(), "plane") == 0) lighttypetemp = LIGHT_TYPE_PLANE;
		if (strcmp(jsonlights[i]["type"].GetString(), "spot") == 0) lighttypetemp = LIGHT_TYPE_SPOT;
		lights.push_back(new Light(
				positiontemp,
				forwardtemp,
				intensitytemp,
				colortemp,
				smrtemp,
				lighttypetemp));
	}
	meshes = std::vector<Mesh*>();
	const rapidjson::Value& jsonmeshes = jsondoc["meshes"];
	const char* filepathtemp;
	glm::vec3 scaletemp;
	glm::quat rotationtemp;
	glm::vec2 uvpositiontemp, uvscaletemp;
	float uvrotationtemp;
	int dres, nres, hres;
	TextureInfo* textemp;
	for (rapidjson::SizeType i = 0; i < jsonmeshes.Size(); i++) {
		filepathtemp = jsonmeshes[i]["filepath"].GetString();
		sscanf(jsonmeshes[i]["position"].GetString(), "%f %f %f", &positiontemp[0], &positiontemp[1], &positiontemp[2]);
		sscanf(jsonmeshes[i]["scale"].GetString(), "%f %f %f", &scaletemp[0], &scaletemp[1], &scaletemp[2]);
		sscanf(jsonmeshes[i]["rotation"].GetString(), "%f %f %f %f", &rotationtemp[0], &rotationtemp[1],
			   &rotationtemp[2], &rotationtemp[3]);
		const rapidjson::Value& jsontextures = jsonmeshes[i]["textures"];
		for (rapidjson::SizeType j = 0; j < jsontextures.Size(); j++) {
			if (strcmp(jsontextures[j]["type"].GetString(), "diffuse") == 0)
				dres = jsontextures[j]["resolution"].GetInt();
			if (strcmp(jsontextures[j]["type"].GetString(), "normal") == 0)
				nres = jsontextures[j]["resolution"].GetInt();
			if (strcmp(jsontextures[j]["type"].GetString(), "height") == 0)
				hres = jsontextures[j]["resolution"].GetInt();
		}
		meshes.push_back(new Mesh(
				filepathtemp,
				positiontemp,
				scaletemp,
				rotationtemp,
				dres, nres, hres));
		for (rapidjson::SizeType j = 0; j < jsontextures.Size(); j++) {
			sscanf(jsontextures[j]["position"].GetString(), "%f %f", &uvpositiontemp[0], &uvpositiontemp[1]);
			sscanf(jsontextures[j]["scale"].GetString(), "%f %f", &uvscaletemp[0], &uvscaletemp[1]);
			uvrotationtemp = jsontextures[j]["rotation"].GetFloat();
			if (strcmp(jsontextures[j]["type"].GetString(), "diffuse") == 0)
				textemp = meshes.back()->getDiffuseTexturePtr();
			if (strcmp(jsontextures[j]["type"].GetString(), "normal") == 0)
				textemp = meshes.back()->getNormalTexturePtr();
			if (strcmp(jsontextures[j]["type"].GetString(), "height") == 0)
				textemp = meshes.back()->getHeightTexturePtr();
			textemp->setUVPosition(uvpositiontemp);
			textemp->setUVScale(uvscaletemp);
			textemp->setUVRotation(uvrotationtemp);
		}
	}

	std::vector<glm::vec3> steppearea = {glm::vec3(-10., 0., -10), glm::vec3(-10, 0., 10), glm::vec3(10., 0., 10.),
										 glm::vec3(10., 0., -10.)};
	meshes[0]->generateSteppeMesh(steppearea,
								  {{glm::vec3(11., 0., 11.),
									glm::vec3(10., 0., -8.),
									glm::vec3(4., 0., -6.),
									glm::vec3(4., 0., 4.),
									glm::vec3(-4., 0., 6.),
									glm::vec3(-6., 0., 4.),
									glm::vec3(-10., 0., -4),
									glm::vec3(-11., 0., 4.)}},
								  0.f);
	CoarseRockSetVars v = {glm::vec4(0, 0, 0, 1), glm::vec4(1, 1, 0.9, 1), glm::vec4(0.9, 0.9, 0.9, 1), glm::vec4(0, 0, 0, 1)};
	TextureHandler::generateTextures({*meshes[0]->getDiffuseTexturePtr(), *meshes[0]->getNormalTexturePtr(), *meshes[0]->getHeightTexturePtr(), *meshes[0]->getSpecularTexturePtr()}, TextureHandler::coarseRockTexGenSet, &v);
	meshes[0]->getDiffuseTexturePtr()->setUVScale(glm::vec2(1));
	meshes[0]->getNormalTexturePtr()->setUVScale(glm::vec2(5));
	meshes[0]->getNormalTexturePtr()->setUVRotation(3.14 * 30 / 180);
	meshes[0]->getHeightTexturePtr()->setUVScale(glm::vec2(0.1));
	meshes[0]->rewriteTextureDescriptorSets();

	ocean = new Ocean(glm::vec3(-10., -2., -10.), glm::vec2(20.), meshes[0]);
//	grass = new ParticleSystem<GrassParticle>(GRASS, 100, ENFORCED_UNIFORM_ON_MESH, {meshes[0]});
	lights[0]->setWorldSpaceSceneBB(meshes[0]->getMin(), meshes[0]->getMax());
}

void WanglingEngine::genScene () {
	meshes.push_back(Mesh::generateBoulder(ROCK_TYPE_GRANITE, glm::vec3(1.f), 0u));
}

void WanglingEngine::updatePCsAndBuffers() {
	testterrain->getGenPushConstantsPtr()->cameravp = primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix(); 
	testterrain->getGenPushConstantsPtr()->numleaves = testterrain->getNumLeaves();

	troubleshootinglines.updatePCs({primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix()});

	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & CAMERA_POSITION_CHANGE_FLAG_BIT
		|| GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & CAMERA_LOOK_CHANGE_FLAG_BIT) {
		for (auto& m : meshes) {
			m->pcdata.cameravpmatrices = primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();
			m->pcdata.camerapos = primarycamera->getPosition() + glm::vec3(0., 0.5, 0.);
			m->pcdata.standinguv = physicshandler.getStandingUV();
		}

		ocean->getPushConstantsPtr()->cameravpmatrices = primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();
		ocean->getPushConstantsPtr()->cameraposition = primarycamera->getPosition() + glm::vec3(0., 0.5, 0.);
		GraphicsHandler::vulkaninfo.grasspushconstants = primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();
		testterrain->getGenPushConstantsPtr()->camerapos = primarycamera->getPosition() + glm::vec3(0., 0.5, 0.);
	}

	// wtf why does commenting out this lock on cause a val err & crash???
	// TODO: what the hell is going on here??
	GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] |= CAMERA_LOOK_CHANGE_FLAG_BIT;
	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & CAMERA_LOOK_CHANGE_FLAG_BIT
		|| GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & LIGHT_CHANGE_FLAG_BIT) {
		GraphicsHandler::vulkaninfo.skyboxpushconstants = {
				primarycamera->getProjectionMatrix() * primarycamera->calcAndGetSkyboxViewMatrix(),
				lights[0]->getPosition()
		};
//		LightUniformBuffer lightuniformbuffertemp[MAX_LIGHTS];
//		for (uint8_t i = 0; i < lights.size(); i++) {
//			if (lights[i]->getShadowType() == SHADOW_TYPE_LIGHT_SPACE_PERSPECTIVE ||
//				lights[i]->getShadowType() == SHADOW_TYPE_CAMERA_SPACE_PERSPECTIVE) {  // this is a lie
//				glm::vec3* tempb = new glm::vec3[8];
//				GraphicsHandler::makeRectPrism(&tempb, meshes[0]->getMin(), meshes[0]->getMax());
//				lights[i]->updateMatrices(primarycamera->getForward(),
//										  primarycamera->getPosition() + glm::vec3(0., 0.5, 0.), tempb, 8);
//				delete[] tempb;
//				lightuniformbuffertemp[i] = {
//						lights[i]->getShadowPushConstantsPtr()->lightvpmatrices,
//						lights[i]->getShadowPushConstantsPtr()->lspmatrix,
//						lights[i]->getPosition(),
//						lights[i]->getIntensity(),
//						glm::vec3(0.0f, 0.0f, 0.0f),    //remember to change this if we need a forward vec
//						lights[i]->getType(),
//						lights[i]->getColor()
//				};
//				GraphicsHandler::VKHelperUpdateUniformBuffer(   // feels like this could be moved a level down, watch out for swapping if and for
//						MAX_LIGHTS,
//						sizeof(LightUniformBuffer),
//						lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
//						&lightuniformbuffertemp[0]);
//			}
//		}
	}

	GraphicsHandler::vulkaninfo.terrainpushconstants =
			primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();

	// how is this not redundant w/ the above??
//	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & LIGHT_CHANGE_FLAG_BIT) {
//		LightUniformBuffer lightuniformbuffertemp[MAX_LIGHTS];
//		for (uint8_t i = 0; i < lights.size(); i++) {
//			lightuniformbuffertemp[i] = {
//					lights[i]->getShadowPushConstantsPtr()->lightvpmatrices,
//					lights[i]->getShadowPushConstantsPtr()->lspmatrix,
//					lights[i]->getPosition(),
//					lights[i]->getIntensity(),
//					glm::vec3(0.0f, 0.0f, 0.0f),    //remember to change this if we need a forward vec
//					lights[i]->getType(),
//					lights[i]->getColor()
//			};
//			GraphicsHandler::VKHelperUpdateUniformBuffer(
//					MAX_LIGHTS,
//					sizeof(LightUniformBuffer),
//					lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
//					&lightuniformbuffertemp[0]);
//		}
//		GraphicsHandler::vulkaninfo.primarygraphicspushconstants.numlights = lights.size();
//		GraphicsHandler::vulkaninfo.oceanpushconstants.numlights = lights.size();
//	}

	// making this block execute no matter what for shadowmap troubleshooting
	/*
	{
		LightUniformBuffer lightuniformbuffertemp[lights.size()];
		for (uint8_t i = 0; i < lights.size(); i++) {
			glm::vec3 tempb[8], * tempbp = &tempb[0];
			GraphicsHandler::makeRectPrism(&tempbp, meshes[0]->getMin(), meshes[0]->getMax());
			lights[i]->updateMatrices(primarycamera->getForward(),
									  primarycamera->getPosition() + glm::vec3(0., 0.5, 0.),
									  tempbp,
									  8);
			lightuniformbuffertemp[i] = {
					lights[i]->getShadowPushConstantsPtr()->lightvpmatrices,
					lights[i]->getShadowPushConstantsPtr()->lspmatrix,
					lights[i]->getPosition(),
					lights[i]->getIntensity(),
					lights[i]->getForward(),
					lights[i]->getType(),
					lights[i]->getColor()
			};
		}
		GraphicsHandler::VKHelperUpdateUniformBuffer(
				lights.size(),
				sizeof(LightUniformBuffer),
				lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
				&lightuniformbuffertemp[0]);
	}
	*/

	MeshUniformBuffer temp;
	for (auto& m: meshes) {
		// TODO: switch to a const ptr, for less copies
		temp = m->getUniformBufferData();
		/*
		GraphicsHandler::VKHelperUpdateUniformBuffer(
				1,
				sizeof(MeshUniformBuffer),
				m->getUniformBufferMemories()[GraphicsHandler::swapchainimageindex],
				reinterpret_cast<void*>(&temp));
				*/
		GraphicsHandler::VKHelperUpdateUniformBuffer(m->getUniformBuffer(), static_cast<void*>(&temp));
	}


}

void WanglingEngine::initSceneData () {
	vkCreateDescriptorSetLayout(GraphicsHandler::vulkaninfo.logicaldevice, &scenedslcreateinfo, nullptr, &scenedsl);

	GraphicsHandler::VKSubInitUniformBuffer(
			&scenedescriptorsets,
			0,
			sizeof(LightUniformBuffer),
			MAX_LIGHTS,
			&lightuniformbuffers,
			&lightuniformbuffermemories,
			scenedsl);

	VkDescriptorImageInfo imginfo = skyboxtexture.getDescriptorImageInfo();
	for (uint8_t sci = 0; sci < GraphicsHandler::vulkaninfo.numswapchainimages; sci++) {
		VkWriteDescriptorSet wrt {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				scenedescriptorsets[sci],
				1,
				0,
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imginfo,
				nullptr,
				nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &wrt, 0, nullptr);
	}
}

void WanglingEngine::initSkybox () {
	skyboxcommandbuffers = new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo cmdbufallocinfo {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.commandpool,
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		MAX_FRAMES_IN_FLIGHT
	};
	vkAllocateCommandBuffers(GraphicsHandler::vulkaninfo.logicaldevice, &cmdbufallocinfo, skyboxcommandbuffers);

	VkDescriptorSetAllocateInfo descsetallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			1, &GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.scenedsl
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &descsetallocinfo, &skyboxds);
}

void WanglingEngine::updateSkyboxDescriptorSets () {
	GraphicsHandler::updateDescriptorSet(
		skyboxds,
		{0},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		{skyboxtexture.getDescriptorImageInfo()},
		{});
}

void WanglingEngine::recordSkyboxCommandBuffers (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			data.renderpass,
			0,
			data.framebuffer,
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
			&cmdbufinherinfo
	};
	vkBeginCommandBuffer(cb, &cmdbufbegininfo);
	vkCmdBindPipeline(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipeline);
	vkCmdPushConstants(
			cb,
			data.pipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SkyboxPushConstants), data.pushconstantdata);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipelinelayout,
			0,
			1, &data.descriptorset,     //is this theoretically identical to sceneds?
			0, nullptr);
	vkCmdDraw(
			cb,
			36,
			1,
			0,
			0);
	vkEndCommandBuffer(cb);
}

// TODO: figure out which buffer recording setup structs can be made static const for efficiency
// you know, we only have to re-enqueue if the tasks change...
void WanglingEngine::enqueueRecordingTasks () {
	cbRecData tempdata {
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		{},
		nullptr,
		nullptr,
		VK_NULL_HANDLE, VK_NULL_HANDLE, 0u};
	/*
	 * Terrain Generation
	 */
	// only recompute voxels if we've had time to realloc memory
	if (testterrain->getCompFIF() == MAX_FRAMES_IN_FLIGHT + 1
		&& testterrain->isMemoryAvailable() 
		&& (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & (CAMERA_LOOK_CHANGE_FLAG_BIT | CAMERA_POSITION_CHANGE_FLAG_BIT))) {
		testterrain->setCompFIF(GraphicsHandler::vulkaninfo.currentframeinflight);
		// code in this block seems very wrong... why are we doing a phase 0 and a phase 1 every loop???
		tempdata.pipeline = GraphicsHandler::vulkaninfo.terraingencomputepipeline;
		tempdata.descriptorset = testterrain->getComputeDescriptorSet();
			// below is v redundant set
		// can probably handle push constants better in general to coorperate with (relatively) new recording strategy
		testterrain->getGenPushConstantsPtr()->phase = 0;
		tempdata.pushconstantdata = reinterpret_cast<void*>(testterrain->getGenPushConstantsPtr());
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Terrain::recordCompute(tempdata, c);}));

		/*
		VkBufferMemoryBarrier2* membar = new VkBufferMemoryBarrier2 {
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			nullptr,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			testterrain->getTreeBuffer(),
			0u,
			NODE_HEAP_SIZE * NODE_SIZE
		};
		recordingtasks.push(cbRecTask({
			VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			nullptr,
			0,
			0, nullptr,
			1, membar,
			0, nullptr}));
		GraphicsHandler::vulkaninfo.terraingenpushconstants2 = GraphicsHandler::vulkaninfo.terraingenpushconstants;
		GraphicsHandler::vulkaninfo.terraingenpushconstants2.phase = 1;
		tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.terraingenpushconstants2);
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Terrain::recordCompute(tempdata, c);}));
		*/
	}
	/*
	 * Ocean Compute
	 */
	tempdata.pipeline = Ocean::getComputePipeline();
	tempdata.descriptorset = ocean->getComputeDescriptorSet();
	tempdata.pushconstantdata = reinterpret_cast<void*>(physicshandler.getTPtr());
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Ocean::recordCompute(tempdata, c);}));

	/*
	 * Shadowmap Renders
	 */
	// do we need those ds mutexes???
	// try removing them from draws once we have both mesh draws in and are stress testing
	// TODO: check for light change flag
	tempdata.scenedescriptorset = scenedescriptorsets[GraphicsHandler::swapchainimageindex];
	tempdata.pipeline = Mesh::getShadowPipeline();
	tempdata.scenedsmutex = &scenedsmutex;
	for (uint8_t i = 0; i < lights.size(); i++) {
		recordingtasks.push(cbRecTask({
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			lights[i]->getShadowRenderPass(),
			lights[i]->getShadowFramebuffer(),
			{{0, 0}, {static_cast<uint32_t>(lights[i]->getShadowmapResolution()),
			static_cast<uint32_t>(lights[i]->getShadowmapResolution())}},
			1,
			&GraphicsHandler::vulkaninfo.shadowmapclear
		}));
		tempdata.renderpass = lights[i]->getShadowRenderPass();
		tempdata.framebuffer = lights[i]->getShadowFramebuffer();
		tempdata.pushconstantdata = reinterpret_cast<void*>(lights[i]->getShadowPushConstantsPtr());
		for (auto& m: meshes) {
			tempdata.descriptorset = m->getDescriptorSet();
			tempdata.dsmutex = m->getDSMutexPtr();
			tempdata.vertexbuffer = m->getVertexBuffer();
			tempdata.indexbuffer = m->getIndexBuffer();
			tempdata.numtris = m->getTrisPtr()->size();
			recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {
				Mesh::recordShadowDraw(tempdata, c);
			}));
		}
		VkImageMemoryBarrier2KHR* imb = new VkImageMemoryBarrier2KHR {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			lights[i]->getShadowmapPtr()->image,
			{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
		};
		recordingtasks.push(cbRecTask({
			VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			nullptr,
			0,
			0, nullptr,
			0, nullptr,
			1, imb
		}));
	}

	/*
	 * Primary Renderpass
	 */
	recordingtasks.push(cbRecTask({
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.primaryrenderpass,
		GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex],
		{{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
		2,
		&GraphicsHandler::vulkaninfo.primaryclears[0]
	}));
	tempdata.renderpass = GraphicsHandler::vulkaninfo.primaryrenderpass;
	tempdata.framebuffer = GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex];

	/*
	 * Skybox
	 */
	tempdata.descriptorset = skyboxds;
	tempdata.pipeline = GraphicsHandler::vulkaninfo.skyboxgraphicspipeline;
	tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.skyboxpushconstants);
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {recordSkyboxCommandBuffers(tempdata, c);}));

	/*
	 * Meshes
	 */
	tempdata.pipeline = Mesh::getPipeline();
	for (auto& m: meshes) {
		tempdata.pushconstantdata = reinterpret_cast<void*>(&m->pcdata);
		// tempdata.descriptorset = m->getDescriptorSets()[GraphicsHandler::swapchainimageindex];
		tempdata.descriptorset = m->getDescriptorSet();
		tempdata.dsmutex = m->getDSMutexPtr();
		tempdata.vertexbuffer = m->getVertexBuffer();
		tempdata.indexbuffer = m->getIndexBuffer();
		tempdata.numtris = m->getTrisPtr()->size();
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Mesh::recordDraw(tempdata, c);}));
	}

	/*
	 * Troubleshooting Lines
	 */
	tempdata.pipeline = Lines::getPipeline();
	tempdata.pushconstantdata = static_cast<void*>(troubleshootinglines.getPCPtr());
	tempdata.vertexbuffer = troubleshootinglines.getVertexBuffer();
	tempdata.numtris = troubleshootinglines.getNumVertices();
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Lines::recordDraw(tempdata, c);}));

	/*
	 * Troubleshooting Voxels
	 */
	tempdata.pipeline = GraphicsHandler::vulkaninfo.voxeltroubleshootingpipeline;
	tempdata.descriptorset = testterrain->getComputeDescriptorSet();
	tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.terrainpushconstants);
	tempdata.numtris = testterrain->getNumLeaves() * 2u;
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Terrain::recordTroubleshootDraw(tempdata, c);}));

	/*
	 * End Primary Renderpass
	 */
	recordingtasks.push(cbRecTask({
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		VK_NULL_HANDLE,
		SSRR::getFramebuffer(GraphicsHandler::swapchainimageindex),
		{{0, 0}, {0, 0}},
		0, nullptr
	}));



	/*
	 * Screen Copy for Objects using Reflection & Refraction 
	 */
	recordingtasks.push(cbRecTask(SSRR::getPreCopyDependency()));
	recordingtasks.push(cbRecTask([] (VkCommandBuffer& c) {SSRR::recordCopy(c);}));
	recordingtasks.push(cbRecTask([] (VkCommandBuffer& c) {SSRR::recordDepthCopy(c);}));
	recordingtasks.push(cbRecTask(SSRR::getPostCopyDependency()));

	/*
	 * Start Renderpass for Objects using Reflection & Refraction
	 */
	recordingtasks.push(cbRecTask({
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		SSRR::getRenderpass(),
		SSRR::getFramebuffer(GraphicsHandler::swapchainimageindex),
		{{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
		2,
		&GraphicsHandler::vulkaninfo.primaryclears[0]
	}));

	tempdata.renderpass = SSRR::getRenderpass();
	tempdata.framebuffer = SSRR::getFramebuffer(GraphicsHandler::swapchainimageindex);
	tempdata.pipeline = Ocean::getGraphicsPipeline();
	tempdata.pushconstantdata = reinterpret_cast<void*>(ocean->getPushConstantsPtr());
	tempdata.descriptorset = ocean->getDescriptorSet();
	tempdata.vertexbuffer = ocean->getVertexBuffer();
	tempdata.indexbuffer = ocean->getIndexBuffer();
	tempdata.numtris = ocean->getTris().size();
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Ocean::recordDraw(tempdata, c);}));

	/*
	 * End SSRR Renderpass
	 */
	recordingtasks.push(cbRecTask({
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		VK_NULL_HANDLE,
		SSRR::getFramebuffer(GraphicsHandler::swapchainimageindex),
		{{0, 0}, {0, 0}},
		0, nullptr
	}));

	/*
	 * Start Compositing Renderpass
	 */
	recordingtasks.push(cbRecTask({
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.compositingrenderpass,
		GraphicsHandler::vulkaninfo.compositingframebuffers[GraphicsHandler::swapchainimageindex],
		{{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
		1,
		&GraphicsHandler::vulkaninfo.primaryclears[0]
	}));
	tempdata.renderpass = GraphicsHandler::vulkaninfo.compositingrenderpass;
	tempdata.framebuffer = GraphicsHandler::vulkaninfo.compositingframebuffers[GraphicsHandler::swapchainimageindex];

	/*
	 * Bloom
	 */
	if (bloom) {

		/*
		GraphicsHandler::vulkaninfo.pppc[0].op = POST_PROC_OP_DOWNSAMPLE;
		GraphicsHandler::vulkaninfo.pppc[0].exposure = mainsettingsfolder.settings[0].data.range.value;
		GraphicsHandler::vulkaninfo.pppc[0].numinvocs = glm::uvec2(GraphicsHandler::vulkaninfo.swapchainextent.width / 2, GraphicsHandler::vulkaninfo.swapchainextent.height / 2);
		GraphicsHandler::vulkaninfo.pppc[0].scext = glm::uvec2(GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height);
		GraphicsHandler::vulkaninfo.pppc[0].dfdepth = 1;
		tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.pppc[0]);
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {
			GraphicsHandler::recordPostProcCompute(tempdata, c);
		}));

		GraphicsHandler::vulkaninfo.pppc[1].op = POST_PROC_OP_DOWNSAMPLE;
		GraphicsHandler::vulkaninfo.pppc[1].exposure = 0.;
		GraphicsHandler::vulkaninfo.pppc[1].numinvocs = GraphicsHandler::vulkaninfo.pppc[0].numinvocs / 2;
		GraphicsHandler::vulkaninfo.pppc[1].scext = glm::uvec2(GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height);
		GraphicsHandler::vulkaninfo.pppc[1].dfdepth = 2;
		tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.pppc[0]);
		tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.pppc[1]);
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {
			GraphicsHandler::recordPostProcCompute(tempdata, c);
		}));
		
		GraphicsHandler::vulkaninfo.pppc[2].op = POST_PROC_OP_DOWNSAMPLE;
		GraphicsHandler::vulkaninfo.pppc[2].exposure = 0.;
		GraphicsHandler::vulkaninfo.pppc[2].numinvocs = GraphicsHandler::vulkaninfo.pppc[1].numinvocs / 2;
		GraphicsHandler::vulkaninfo.pppc[2].scext = glm::uvec2(GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height);
		GraphicsHandler::vulkaninfo.pppc[2].dfdepth = 3;
		tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.pppc[2]);
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {GraphicsHandler::recordPostProcCompute(tempdata, c);}));
		*/
		const uint8_t maxdfdepth = 5;
		// theres def a better data type for this, refine when we get a better idea of what our postproc will look like
		/*std::vector<cbRecData> postprocdata(maxdfdepth * 2);
		for (uint8_t d = 0; d < maxdfdepth; d++) {
			postprocdata[d].pipeline = GraphicsHandler::vulkaninfo.postprocpipeline;
			postprocdata[d].descriptorset = GraphicsHandler::vulkaninfo.postprocds[GraphicsHandler::swapchainimageindex];
			GraphicsHandler::vulkaninfo.pppc[d].op = POST_PROC_OP_DOWNSAMPLE;
			GraphicsHandler::vulkaninfo.pppc[d].numinvocs = glm::uvec2(GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height) / pow (2, d + 1); // check this, uint math is sketchy here
			GraphicsHandler::vulkaninfo.pppc[d].scext = glm::uvec2(GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height);
			GraphicsHandler::vulkaninfo.pppc[d].dfdepth = d + 1;
			postprocdata[d].pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.pppc[d]);
		}
		*/
		/*
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {
			GraphicsHandler::recordPostProcCompute(tempdata, GraphicsHandler::vulkaninfo.pppc, c);
		}));

		tempdata.pipeline = GraphicsHandler::vulkaninfo.postprocgraphicspipeline;
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {
			GraphicsHandler::recordPostProcGraphics(tempdata, c);
		}));
		*/
	}

	/*
	 * Texture Monitor
	 */
	tempdata.descriptorset = texmon.getDescriptorSet();
	tempdata.pipeline = TextureMonitor::getPipeline();
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {TextureMonitor::recordDraw(tempdata, c);}));

	/*
	 * Upper Left Text
	 */
	tempdata.descriptorset = troubleshootingtext->getDescriptorSets()[GraphicsHandler::vulkaninfo.currentframeinflight];
	tempdata.pipeline = GraphicsHandler::vulkaninfo.textgraphicspipeline;
	tempdata.pushconstantdata = reinterpret_cast<void*>(troubleshootingtext->getPushConstantData());
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Text::recordCommandBuffer(tempdata, c);}));

	// TODO: switch post-proc off of linear min-mag, should have no interp
}

void WanglingEngine::processRecordingTasks (
		std::queue<cbRecTask>* tasks,
		std::queue<cbCollectInfo>* resultcbs,
		uint8_t fifindex,
		uint8_t threadidx,
		VkDevice device) {
	// this is quite a bit of copying, see if we can find a way to avoid later, b/c its neccesary in here anyway
	cbSet* const cbset = &GraphicsHandler::vulkaninfo.threadCbSets[threadidx][fifindex];
	uint8_t bufferidx = 0u;   // watch out for overflow on this
	std::function<void (VkCommandBuffer&)> recfunc;
	std::unique_lock<std::mutex> lock(recordingmutex, std::defer_lock);
	const VkCommandBufferAllocateInfo cballocinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			cbset->pool,
			VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			1u
	};
	while (true) {
		// look in to efficiency modifications w/ dynamic cmd buf alloc
		// order with which secondary buffers are enqueued strongly affects frequency of their alloc/free
		// could store use frequency info in cbSet
		lock.lock();
		if (tasks->empty()) break; // should we unlock here??
		if (tasks->front().type == cbRecTask::cbRecTaskType::CB_REC_TASK_TYPE_RENDERPASS) {
			resultcbs->push(cbCollectInfo(tasks->front().data.rpbi));
			tasks->pop();
			lock.unlock();
			continue;
		}
		if (tasks->front().type == cbRecTask::cbRecTaskType::CB_REC_TASK_TYPE_DEPENDENCY) {
			resultcbs->push(cbCollectInfo(tasks->front().data.di));
			tasks->pop();
			lock.unlock();
			continue;
		}
		recfunc = tasks->front().data.func;
		tasks->pop();
		if (bufferidx == cbset->buffers.size()) {
			if (bufferidx == cbset->buffers.size()) {
				cbset->buffers.push_back(VK_NULL_HANDLE);
				vkAllocateCommandBuffers(device,
										 &cballocinfo,
										 &cbset->buffers.back());
			}
		}
		resultcbs->push(cbCollectInfo(cbset->buffers[bufferidx]));
		lock.unlock();
		recfunc(cbset->buffers[bufferidx]);
		bufferidx++;
	}
	if (bufferidx < cbset->buffers.size()) {
		vkFreeCommandBuffers(device,
							 cbset->pool,
							 cbset->buffers.size() - bufferidx,
							 &cbset->buffers[bufferidx]);
		cbset->buffers.erase(cbset->buffers.begin() + bufferidx, cbset->buffers.end());
	}
}

void WanglingEngine::collectSecondaryCommandBuffers () {
	vkResetCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight], 0); // is this call neccesary???

	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight], &cmdbufferbegininfo);
	bool inrp = false;
	while (!secondarybuffers.empty()) {
		if (secondarybuffers.front().type == cbCollectInfo::cbCollectInfoType::CB_COLLECT_INFO_TYPE_COMMAND_BUFFER) {
			vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
								 1,
								 &secondarybuffers.front().data.cmdbuf);
		} else if (secondarybuffers.front().type == cbCollectInfo::cbCollectInfoType::CB_COLLECT_INFO_TYPE_RENDERPASS) {
			// ending an rp w/o starting another is done by passing a renderpassbi as if to begin, but with a null handle as renderpass
			// check inrp logic here
			if (inrp) vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
			else inrp = true;
			if (secondarybuffers.front().data.rpbi.renderPass != VK_NULL_HANDLE) {
				vkCmdBeginRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
									 &secondarybuffers.front().data.rpbi,
									 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
			} else inrp = false;
		} else if (secondarybuffers.front().type == cbCollectInfo::cbCollectInfoType::CB_COLLECT_INFO_TYPE_DEPENDENCY) {
			if (secondarybuffers.front().data.di.imageMemoryBarrierCount) {
				// TODO: make this work for storage buffer
				if (inrp) {
					vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
					inrp = false;
				}

				// i feel like we should be able to use vkCmdPipelineBarrier2(KHR), but it crashed when I tried...
				GraphicsHandler::pipelineBarrierFromKHR(secondarybuffers.front().data.di);
				delete[] secondarybuffers.front().data.di.pImageMemoryBarriers;
			}
		}
		secondarybuffers.pop();
	}
	if (inrp)
		vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	vkEndCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	GraphicsHandler::changeflags[GraphicsHandler::vulkaninfo.currentframeinflight] = NO_CHANGE_FLAG_BIT;
}

// TODO: clean the fuck out of this function
// move sync stuff to its own GH function and get rid of all the comments
// probably move processing behavior to another function too
void WanglingEngine::draw () {
	std::chrono::time_point<std::chrono::high_resolution_clock> start;

	VkResult acquire = vkAcquireNextImageKHR(
			GraphicsHandler::vulkaninfo.logicaldevice,
			GraphicsHandler::vulkaninfo.swapchain,
			1000000,
			GraphicsHandler::vulkaninfo.imageavailablesemaphores[0],
			VK_NULL_HANDLE,
			&GraphicsHandler::swapchainimageindex);
	if (acquire == VK_TIMEOUT) exit(1);

	// ok so i can see framerate being limited by the 60Hz refresh rate of this screen, but that doesnt really explain getting the same img twice
	// i do want to calc a theoretical framerate that subtracts the time it takes to wait for an image to be done presenting

	vkWaitForFences(
			GraphicsHandler::vulkaninfo.logicaldevice,
			1, &GraphicsHandler::vulkaninfo.submitfinishedfences[GraphicsHandler::vulkaninfo.currentframeinflight],
			VK_FALSE,
			UINT64_MAX);
	/*
	Node * nodesarrptr = reinterpret_cast<Node *>(testterrain->getNodeHeapPtr());
//	nodesarrptr++;
//	while (nodesarrptr->children[0] != 0) {
//		std::cout << "&p = " << nodesarrptr->parent << ", cidx = " << nodesarrptr->childidx << std::endl;
//		nodesarrptr++;
//	}
	Node nodesarr[328];
	memcpy(reinterpret_cast<void *>(&nodesarr[0]), reinterpret_cast<void *>(++nodesarrptr), 328 * 11 * sizeof(uint32_t));
	*/
	troubleshootingtext->setMessage(GraphicsHandler::troubleshootingsstrm.str(),
									GraphicsHandler::swapchainimageindex);
	GraphicsHandler::troubleshootingsstrm = std::stringstream(std::string());

	//updatePCsAndBuffers();

	glfwPollEvents();
	
	primarycamera->takeInputs(GraphicsHandler::vulkaninfo.window);
	physicshandler.update(meshes[0]->getTris());

	// TODO: refine efficiency & tidiness in this func
	// e.g. still have to update unibuf func to only use one buf and one mem
	updatePCsAndBuffers();

	enqueueRecordingTasks();

	for (uint8_t x = 0; x < NUM_RECORDING_THREADS; x++) {
		recordingthreads[x] = std::thread(
				processRecordingTasks,
				&recordingtasks,
				&secondarybuffers,
				GraphicsHandler::vulkaninfo.currentframeinflight,
				x,
				GraphicsHandler::vulkaninfo.logicaldevice);
	}

	for (uint8_t i = 0; i < NUM_RECORDING_THREADS; i++) {
		recordingthreads[i].join();
	}

	collectSecondaryCommandBuffers();

	start = std::chrono::high_resolution_clock::now();

	// this function comes out to take ~ 0.012 or 0.013 seconds!
	// and when we ran on my monitor @ 75Hz refresh rate, avg fps jumped up to 75 and this function took more like 0.007 to 0.009 seconds!
	// TODO: time this func to find out exactly what call results in this time
	// need to be able to separate gpu load from imposed refresh rate limitation
	GraphicsHandler::submitAndPresent();

	calcFrameStats();

	GraphicsHandler::troubleshootingsstrm << (1. / rendertimemean) << " avg fps (SD " << (1. / rendertimesd) << ")"
										  << "\ntheoretically could be as high as " << (1. / (rendertimemean -
																							  std::chrono::duration<float>(
																									  std::chrono::high_resolution_clock::now() -
																									  start).count()))
										  << "\nswapchain image: " << GraphicsHandler::swapchainimageindex
										  << " frame in flight: "
										  << GraphicsHandler::vulkaninfo.currentframeinflight
										  << "\nchangeflags: "
										  << std::bitset<8>(GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]);



	// which one would be more efficient, modulus or ternary bounds check?
	GraphicsHandler::vulkaninfo.currentframeinflight =
			(GraphicsHandler::vulkaninfo.currentframeinflight + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool WanglingEngine::shouldClose () {
	return glfwWindowShouldClose(GraphicsHandler::vulkaninfo.window)
		   || glfwGetKey(GraphicsHandler::vulkaninfo.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

void WanglingEngine::calcFrameStats (float sptime) {
	if (framesamplecounter == NUM_FRAME_SAMPLES - 1) {
		rendertimemean = 0.;
		for (uint8_t i = 0u; i < NUM_FRAME_SAMPLES; i++) {
			rendertimemean += rendertimes[i];
		}
		rendertimesd = 0.;
		for (uint8_t i = 0u; i < NUM_FRAME_SAMPLES; i++) {
			rendertimesd += pow(rendertimes[i] - rendertimemean, 2);
		}
		rendertimesd = sqrt(rendertimesd / (float)NUM_FRAME_SAMPLES);
		rendertimemean /= (float)NUM_FRAME_SAMPLES;
		framesamplecounter = 0u;
	} else {
		framesamplecounter++;
	}
	rendertimes[framesamplecounter] = *physicshandler.getDtPtr() - sptime;
}
