//
// Created by Daniel Paavola on 7/22/21.
//

#include "WanglingEngine.h"

std::condition_variable WanglingEngine::conditionvariable = std::condition_variable();
std::mutex WanglingEngine::submitfencemutex = std::mutex();
std::mutex WanglingEngine::recordingmutex = std::mutex();
bool WanglingEngine::submitfenceavailable = true;

WanglingEngine::WanglingEngine () {
	TextureHandler::init();
	uint8_t nummeshes, numlights;
	countSceneObjects("../resources/scenelayouts/rocktestlayout.json", &nummeshes, &numlights);
	// TODO: figure out best value for below arg
	GraphicsHandler::VKInit(100);
	GraphicsHandler::VKInitPipelines();
	loadScene("../resources/scenelayouts/rocktestlayout.json");
	genScene();

	physicshandler = PhysicsHandler(primarycamera);

	// TODO: move this array creation to somewhere in Mesh
	/* Honestly, we need to find a more elegant way to handle Mesh texture generation via TextureHandler. A few options
	 * to consider:
	 * 1) Get rid of TextureHandler and move generation functions to Mesh. This seems easy, but i actually think that
	 *    having it as a seperate object may help later, especially if/when we get to dynamic texture generation and
	 *    possibly texture generation multithreading.
	 * 2) Make some revisions above, but ultimately settle for a mediocre-looking solution. No.
	 * 3) Make a Mesh function to handle all this that takes a function pointer from texture handler and variadic args.
	 * 4) Something else???
	 * To really find the best solution, we need to consider how TextureHandler is going to be used, what particular
	 * function it has, other than some vague hope that it will help with efficiency.
	 */
	// diff btwn normalmap and normaltex??????
//	TextureInfo* textemp = ocean->getNormalMapPtr();
//	texturehandler.generateOceanTextures(&textemp, 1);
	TextureHandler::generateTextures({*ocean->getNormalMapPtr()}, TextureHandler::oceanTexGenSet);
	ocean->rewriteTextureDescriptorSets();

	troubleshootingtext = new Text(
			"troubleshooting text",
			glm::vec2(-1.0f, -1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			72.0f,
			GraphicsHandler::vulkaninfo.horizontalres,
			GraphicsHandler::vulkaninfo.verticalres);

	initSkybox();

	GraphicsHandler::VKHelperInitTexture(
			&skyboxtexture,
			128, 0,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_CUBEMAP,
			VK_IMAGE_VIEW_TYPE_CUBE,
			GraphicsHandler::linearminmagsampler);
	texturehandler.generateSkyboxTexture(&skyboxtexture);
	updateSkyboxDescriptorSets();

	initSceneData();

	initTroubleshootingLines();

	updateTexMonDescriptorSets(*meshes[1]->getDiffuseTexturePtr());

	// TODO: move to shadowsamplerinit func
	for (uint8_t scii = 0; scii < GraphicsHandler::vulkaninfo.numswapchainimages; scii++) {
		VkWriteDescriptorSet shadowsamplerwrites[lights.size()];
		VkDescriptorImageInfo imginfo;
		for (uint8_t m = 0; m < meshes.size(); m++) {
			for (uint8_t l = 0; l < lights.size(); l++) {
				if (l < lights.size()) imginfo = lights[l]->getShadowmapPtr()->getDescriptorImageInfo();
				else imginfo = {};
				shadowsamplerwrites[l] = {
						VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						nullptr,
						meshes[m]->getDescriptorSets()[scii],
						4,
						l,
						1,
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						&imginfo,
						nullptr,
						nullptr
				};
			}
			vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
								   lights.size(), &shadowsamplerwrites[0],
								   0, nullptr);
		}
	}

	for (uint8_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
//		recordingthreads[0] = std::thread(threadedCmdBufRecord, this);
		enqueueRecordingTasks();
		recordingthreads[0] = std::thread(processRecordingTasks, &recordingtasks);
		recordingthreads[0].join();
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
					glm::vec3(0.0f, 0.0f, 0.0f),    //remember to change this if we need a forward vec
					lights[x]->getType(),
					lights[x]->getColor()
			};
		}
		GraphicsHandler::VKHelperUpdateUniformBuffer(
				MAX_LIGHTS,
				sizeof(LightUniformBuffer),
				lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
				&lightuniformbuffertemp[0]);
		GraphicsHandler::vulkaninfo.primarygraphicspushconstants.numlights = lights.size();
		GraphicsHandler::vulkaninfo.oceanpushconstants.numlights = lights.size();
	}
	GraphicsHandler::swapchainimageindex = 0;
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

//	std::vector<glm::vec3> steppearea = {glm::vec3(-10., 0., -10), glm::vec3(-10, 0., 10), glm::vec3(10., 0., 10.),
//										 glm::vec3(10., 0., -10.)};
//	meshes[0]->generateSteppeMesh(steppearea,
//								  {{glm::vec3(11., 0., 11.),
//									glm::vec3(10., 0., -8.),
//									glm::vec3(4., 0., -6.),
//									glm::vec3(4., 0., 4.),
//									glm::vec3(-4., 0., 6.),
//									glm::vec3(-6., 0., 4.),
//									glm::vec3(-10., 0., -4),
//									glm::vec3(-11., 0., 4.)}},
//								  0.f);
//	TextureHandler::generateNewSystemTextures({*(meshes[0]->getDiffuseTexturePtr()),
//											   *(meshes[0]->getNormalTexturePtr()),
//											   *(meshes[0]->getHeightTexturePtr())});
//	meshes[0]->getDiffuseTexturePtr()->setUVScale(glm::vec2(1.f, 1.f));
//	meshes[0]->getDiffuseTexturePtr()->setUVPosition(glm::vec2(0.f, 0.f));
//	meshes[0]->rewriteTextureDescriptorSets();
	ocean = new Ocean(glm::vec3(-10., -2., -10.), glm::vec2(20.), meshes[0]);
	grass = new ParticleSystem<GrassParticle>(GRASS, 100, ENFORCED_UNIFORM_ON_MESH, {meshes[0]});
	lights[0]->setWorldSpaceSceneBB(meshes[0]->getMin(), meshes[0]->getMax());
}

void WanglingEngine::genScene () {
	meshes.push_back(Mesh::generateBoulder(ROCK_TYPE_GRANITE, glm::vec3(1.f), 0u));
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

	skyboxdescriptorsets = new VkDescriptorSet[GraphicsHandler::vulkaninfo.numswapchainimages];
	VkDescriptorSetLayout descsetlay[GraphicsHandler::vulkaninfo.numswapchainimages];
	for (uint8_t x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++)
		descsetlay[x] = GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.scenedsl;
	VkDescriptorSetAllocateInfo descsetallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			GraphicsHandler::vulkaninfo.numswapchainimages,
			&descsetlay[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &descsetallocinfo, &skyboxdescriptorsets[0]);

	texmoncommandbuffers = new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo cbai {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.commandpool,
			VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			MAX_FRAMES_IN_FLIGHT
	};
	vkAllocateCommandBuffers(GraphicsHandler::vulkaninfo.logicaldevice, &cbai, texmoncommandbuffers);

	texmondescriptorsets = new VkDescriptorSet[GraphicsHandler::vulkaninfo.numswapchainimages];
	VkDescriptorSetLayout dsl[GraphicsHandler::vulkaninfo.numswapchainimages];
	for (uint8_t x = 0; x < GraphicsHandler::vulkaninfo.numswapchainimages; x++)
		dsl[x] = GraphicsHandler::vulkaninfo.texmongraphicspipeline.objectdsl;
	VkDescriptorSetAllocateInfo dsallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			GraphicsHandler::vulkaninfo.numswapchainimages,
			&dsl[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &dsallocinfo, &texmondescriptorsets[0]);
}

void WanglingEngine::initTroubleshootingLines () {
	std::vector<Vertex> troubleshootinglinespoints;
	std::vector<glm::vec3> controlpoints {
			glm::vec3(11., 0., 11.),
			glm::vec3(10., 0., -8.),
			glm::vec3(4., 0., -6.),
			glm::vec3(4., 0., 4.),
			glm::vec3(-4., 0., 6.),
			glm::vec3(-6., 0., 4.),
			glm::vec3(-10., 0., -4),
			glm::vec3(-11., 0., 4.)
	};
	glm::vec3 splinePos;
	for (uint8_t p1 = 1; p1 + 2 < controlpoints.size(); p1++) {
		for (uint8_t x = 0; x < 25; x++) {
			splinePos = PhysicsHandler::catmullRomSplineAtMatrified(
					controlpoints,
					p1,
					0.5,
					float_t(x) / 24.f);
			troubleshootinglinespoints.push_back({
														 splinePos,
														 glm::vec3(0., 0., 0.),
														 glm::vec2(0., 0.)
												 });
			troubleshootinglinespoints.push_back({
														 splinePos,
														 glm::vec3(0., 0., 0.),
														 glm::vec2(0., 0.)
												 });

		}
	}
	troubleshootinglinespoints.erase(troubleshootinglinespoints.begin());
	troubleshootinglinespoints.erase(troubleshootinglinespoints.end() - 1);

	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0.5, 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0.5, 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});
	troubleshootinglinespoints.push_back({glm::vec3(0., 0., 0.5), glm::vec3(0., 0., 0.), glm::vec2(0., 0.)});

	GraphicsHandler::VKHelperInitVertexBuffer(
			troubleshootinglinespoints,
			&troubleshootinglinesvertexbuffer,
			&troubleshootinglinesvertexbuffermemory);

	troubleshootinglinescommandbuffers = new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo cmdbufallocinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.commandpool,
			VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			MAX_FRAMES_IN_FLIGHT
	};
	vkAllocateCommandBuffers(GraphicsHandler::vulkaninfo.logicaldevice, &cmdbufallocinfo,
							 troubleshootinglinescommandbuffers);
}

void WanglingEngine::updateSkyboxDescriptorSets () {
	VkDescriptorImageInfo imginfo = skyboxtexture.getDescriptorImageInfo();
	for (uint8_t sci = 0; sci < GraphicsHandler::vulkaninfo.numswapchainimages; sci++) {
		VkWriteDescriptorSet writedescset {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				skyboxdescriptorsets[sci],
				0,
				0,
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imginfo,
				nullptr,
				nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);
	}
}

void WanglingEngine::recordSkyboxCommandBuffers (uint8_t fifindex, uint8_t sciindex) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.primaryrenderpass,
			0,
			GraphicsHandler::vulkaninfo.primaryframebuffers[sciindex],
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
	vkBeginCommandBuffer(skyboxcommandbuffers[fifindex], &cmdbufbegininfo);
	vkCmdBindPipeline(
			skyboxcommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipeline);
	vkCmdPushConstants(
			skyboxcommandbuffers[fifindex],
			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SkyboxPushConstants),
			&GraphicsHandler::vulkaninfo.skyboxpushconstants);
	vkCmdBindDescriptorSets(
			skyboxcommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipelinelayout,
			0,
			1, &skyboxdescriptorsets[sciindex],     //is this theoretically identical to sceneds?
			0, nullptr);
	vkCmdDraw(
			skyboxcommandbuffers[fifindex],
			36,
			1,
			0,
			0);
	vkEndCommandBuffer(skyboxcommandbuffers[fifindex]);
}

void WanglingEngine::recordSkyboxCommandBuffers (cbRecData data) {
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
	vkBeginCommandBuffer(data.commandbuffer, &cmdbufbegininfo);
	vkCmdBindPipeline(
			data.commandbuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipeline);
	vkCmdPushConstants(
			data.commandbuffer,
			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SkyboxPushConstants),
			&GraphicsHandler::vulkaninfo.skyboxpushconstants);
	vkCmdBindDescriptorSets(
			data.commandbuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipelinelayout,
			0,
			1, &data.descriptorset,     //is this theoretically identical to sceneds?
			0, nullptr);
	vkCmdDraw(
			data.commandbuffer,
			36,
			1,
			0,
			0);
	vkEndCommandBuffer(data.commandbuffer);
}

void WanglingEngine::recordTexMonCommandBuffers (uint8_t fifindex, uint8_t sciindex) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.primaryrenderpass,
			0,
			GraphicsHandler::vulkaninfo.primaryframebuffers[sciindex],
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
	vkBeginCommandBuffer(texmoncommandbuffers[fifindex], &cmdbufbegininfo);
	vkCmdBindPipeline(
			texmoncommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.texmongraphicspipeline.pipeline);
//	vkCmdPushConstants(
//			texmoncommandbuffers[fifindex],
//			GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipelinelayout,
//			VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
//			0,
//			sizeof(SkyboxPushConstants),
//			&GraphicsHandler::vulkaninfo.skyboxpushconstants);
	vkCmdBindDescriptorSets(
			texmoncommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.texmongraphicspipeline.pipelinelayout,
			0,
			1, &texmondescriptorsets[sciindex],
			0, nullptr);
	vkCmdDraw(
			texmoncommandbuffers[fifindex],
			6,
			1,
			0,
			0);
	vkEndCommandBuffer(texmoncommandbuffers[fifindex]);
}

void WanglingEngine::recordTroubleshootingLinesCommandBuffers (
		uint8_t fifindex,
		uint8_t sciindex,
		WanglingEngine* self) {
	VkCommandBufferInheritanceInfo cbii {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.primaryrenderpass,
			0,
			GraphicsHandler::vulkaninfo.primaryframebuffers[sciindex],
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cbbi {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
			&cbii
	};
	vkBeginCommandBuffer(
			troubleshootinglinescommandbuffers[fifindex],
			&cbbi);
	vkCmdBindPipeline(
			troubleshootinglinescommandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.linegraphicspipeline.pipeline);
	vkCmdPushConstants(
			troubleshootinglinescommandbuffers[fifindex],
			GraphicsHandler::vulkaninfo.linegraphicspipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4),
			&GraphicsHandler::vulkaninfo.grasspushconstants);
	VkDeviceSize offsettemp = 0u;
	vkCmdBindVertexBuffers(
			troubleshootinglinescommandbuffers[fifindex],
			0,
			1,
			&self->troubleshootinglinesvertexbuffer,
			&offsettemp);
	vkCmdDraw(
			troubleshootinglinescommandbuffers[fifindex],
			254,
			1,
			0,
			0);
	vkEndCommandBuffer(troubleshootinglinescommandbuffers[fifindex]);
}

void WanglingEngine::updateTexMonDescriptorSets (TextureInfo tex) {
	VkDescriptorImageInfo imginfo = tex.getDescriptorImageInfo();
	for (uint8_t sci = 0; sci < GraphicsHandler::vulkaninfo.numswapchainimages; sci++) {
		VkWriteDescriptorSet writedescset {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				texmondescriptorsets[sci],
				0,
				0,
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imginfo,
				nullptr,
				nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);
	}
}

void WanglingEngine::recordCommandBuffer (WanglingEngine* self, uint32_t fifindex) {
	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] | LIGHT_CHANGE_FLAG_BIT) {
		for (uint8_t i = 0; i < self->lights.size(); i++) {
			for (auto& m: self->meshes)
				m->recordShadowDraw(
						fifindex,
						GraphicsHandler::swapchainimageindex,
						self->lights[i]->getShadowRenderPass(),
						self->lights[i]->getShadowFramebuffer(),
						i,
						self->lights[i]->getShadowPushConstantsPtr());
		}
	}

	self->ocean->recordCompute(fifindex);
	self->recordSkyboxCommandBuffers(fifindex, GraphicsHandler::swapchainimageindex);
	for (auto& m: self->meshes)
		m->recordDraw(fifindex,
					  GraphicsHandler::swapchainimageindex,
					  self->scenedescriptorsets);
	self->ocean->recordDraw(fifindex, GraphicsHandler::swapchainimageindex, self->scenedescriptorsets);
	self->grass->recordDraw(fifindex, GraphicsHandler::swapchainimageindex, self->scenedescriptorsets);
	self->recordTexMonCommandBuffers(fifindex, GraphicsHandler::swapchainimageindex);
	self->troubleshootingtext->recordCommandBuffer(fifindex, GraphicsHandler::swapchainimageindex);
	self->recordTroubleshootingLinesCommandBuffers(fifindex, GraphicsHandler::swapchainimageindex, self);

	vkResetCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 0);

	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], &cmdbufferbegininfo);

	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] | LIGHT_CHANGE_FLAG_BIT) {
		for (uint8_t i = 0; i < self->lights.size(); i++) {
			VkClearValue clearval {1., 0};
			VkRenderPassBeginInfo renderpassbegininfo {
					VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					nullptr,
					self->lights[i]->getShadowRenderPass(),
					self->lights[i]->getShadowFramebuffer(),
					{{0, 0}, {static_cast<uint32_t>(self->lights[i]->getShadowmapResolution()),
							  static_cast<uint32_t>(self->lights[i]->getShadowmapResolution())}},
					1,
					&clearval
			};
			vkCmdBeginRenderPass(
					GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
					&renderpassbegininfo,
					VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
			for (auto& m: self->meshes)
				vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
									 &m->getShadowCommandBuffers()[i][fifindex]);
			vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[fifindex]);
			VkImageMemoryBarrier imgmembar {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					self->lights[i]->getShadowmapPtr()->image,
					{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
			};
			vkCmdPipelineBarrier(GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
								 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
								 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &imgmembar);
		}
	}

	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
						 &self->ocean->getComputeCommandBuffers()[fifindex]);

	VkRenderPassBeginInfo renderpassbegininfo {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.primaryrenderpass,
			GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex],
			{{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
			2,
			&GraphicsHandler::vulkaninfo.primaryclears[0]
	};
	vkCmdBeginRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], &renderpassbegininfo,
						 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
						 &self->skyboxcommandbuffers[fifindex]);

	for (auto& m: self->meshes)
		vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
							 &m->getCommandBuffers()[fifindex]);

	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
						 &self->grass->getCommandBuffers()[fifindex]);

	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
						 &self->texmoncommandbuffers[fifindex]);

	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
						 &self->troubleshootingtext->getCommandBuffers()[fifindex]);

	vkCmdExecuteCommands(
			GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
			1,
			&self->troubleshootinglinescommandbuffers[fifindex]);


	/*
	 * Summary of images and their neccesary layout transitions
	 * SCI is current swapchain image, SSRR is current ssrr buffer image
	 *
	 * SCI must be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to start and until the end of primary renderpass
	 *
	 * SSRR must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL before the cpy
	 * SCI must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL before the cpy
	 * then do cpy
	 * SSRR must be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL before the start of the water renderpass
	 * SCI must be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL before the start of the water renderpass
	 *
	 * SCI must be VK_IMAGE_LAYOUT_PRESENT_SRC_KHR before the submission of the queue
	 *
	 * OR we could run a test with both just VK_IMAGE_LAYOUT_GENERAL
	 */

	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
						 1, &self->ocean->getCommandBuffers()[fifindex]);

	vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[fifindex]);

	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] | LIGHT_CHANGE_FLAG_BIT) {
		// ummmmm, shouldn't this layout change come before we render meshes???
		for (uint8_t i = 0; i < self->lights.size(); i++) {
			VkImageMemoryBarrier imgmembar {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					self->lights[i]->getShadowmapPtr()->image,
					{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
			};
			vkCmdPipelineBarrier(GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
								 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &imgmembar);
		}
	}
	vkEndCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[fifindex]);
	GraphicsHandler::changeflags[fifindex] = NO_CHANGE_FLAG_BIT;
}

void WanglingEngine::enqueueRecordingTasks () {
	cbRecData tempdata {GraphicsHandler::vulkaninfo.primaryrenderpass,
						GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex],
						skyboxcommandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
						skyboxdescriptorsets[GraphicsHandler::swapchainimageindex]};
	// TODO: get rid of this wacky cast syntax after we are no longer overloading
	recordingtasks.push(std::bind(static_cast<void (*) (cbRecData)>(recordSkyboxCommandBuffers), tempdata));
}

void WanglingEngine::processRecordingTasks (std::queue<std::function<void ()>>* tasks) {
	// this is quite a bit of copying, see if we can find a way to avoid later, b/c its neccesary in here anyway
	std::function<void ()> temp;
	std::unique_lock<std::mutex> lock(recordingmutex, std::defer_lock);
	while (!tasks->empty()) {
		lock.lock();
		temp = tasks->front();
		tasks->pop();
		lock.unlock();
		temp();
	}
}

void WanglingEngine::collectSecondaryCommandBuffers () {
	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
						 &cmdbufferbegininfo);
	VkRenderPassBeginInfo renderpassbegininfo {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.primaryrenderpass,
			GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex],
			{{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
			2,
			&GraphicsHandler::vulkaninfo.primaryclears[0]
	};
	vkCmdBeginRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
						 &renderpassbegininfo,
						 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
						 1,
						 &skyboxcommandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	vkEndCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	GraphicsHandler::changeflags[GraphicsHandler::vulkaninfo.currentframeinflight] = NO_CHANGE_FLAG_BIT;
}

void WanglingEngine::draw () {
	std::chrono::time_point<std::chrono::high_resolution_clock> start;

	TIME_OPERATION(vkAcquireNextImageKHR(
			GraphicsHandler::vulkaninfo.logicaldevice,
			GraphicsHandler::vulkaninfo.swapchain,
			UINT64_MAX,
			GraphicsHandler::vulkaninfo.imageavailablesemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
			VK_NULL_HANDLE,
			&GraphicsHandler::swapchainimageindex));

	TIME_OPERATION(vkWaitForFences(
			GraphicsHandler::vulkaninfo.logicaldevice,
			1,
			&GraphicsHandler::vulkaninfo.submitfinishedfences[GraphicsHandler::vulkaninfo.currentframeinflight],
			VK_FALSE,
			UINT64_MAX));
	troubleshootingtext->setMessage(GraphicsHandler::troubleshootingsstrm.str(), GraphicsHandler::swapchainimageindex);
	GraphicsHandler::troubleshootingsstrm = std::stringstream(std::string());

	enqueueRecordingTasks();

	recordingthreads[0] = std::thread(processRecordingTasks, &recordingtasks);

//	recordingthreads[0] = std::thread(threadedCmdBufRecord, this);

	glfwPollEvents();

	start = std::chrono::high_resolution_clock::now();

	primarycamera->takeInputs(GraphicsHandler::vulkaninfo.window);
	physicshandler.update(meshes[0]->getTris());
	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & CAMERA_POSITION_CHANGE_FLAG_BIT
		|| GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & CAMERA_LOOK_CHANGE_FLAG_BIT) {
		GraphicsHandler::vulkaninfo.primarygraphicspushconstants.cameravpmatrices =
				primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();
		GraphicsHandler::vulkaninfo.primarygraphicspushconstants.camerapos =
				primarycamera->getPosition() + glm::vec3(0., 0.5, 0.);
		GraphicsHandler::vulkaninfo.primarygraphicspushconstants.standinguv = physicshandler.getStandingUV();

		GraphicsHandler::vulkaninfo.oceanpushconstants.cameravpmatrices =
				primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();
		GraphicsHandler::vulkaninfo.oceanpushconstants.cameraposition =
				primarycamera->getPosition() + glm::vec3(0., 0.5, 0.);

		GraphicsHandler::vulkaninfo.grasspushconstants =
				primarycamera->getProjectionMatrix() * primarycamera->getViewMatrix();
	}

	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & CAMERA_LOOK_CHANGE_FLAG_BIT
		|| GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & LIGHT_CHANGE_FLAG_BIT) {
		GraphicsHandler::vulkaninfo.skyboxpushconstants = {
				primarycamera->getProjectionMatrix() * primarycamera->calcAndGetSkyboxViewMatrix(),
				lights[0]->getPosition()
		};
		LightUniformBuffer lightuniformbuffertemp[MAX_LIGHTS];
		for (uint8_t i = 0; i < lights.size(); i++) {
			if (lights[i]->getShadowType() == SHADOW_TYPE_LIGHT_SPACE_PERSPECTIVE) {
				glm::vec3* tempb = new glm::vec3[8];
				GraphicsHandler::makeRectPrism(&tempb, meshes[0]->getMin(), meshes[0]->getMax());
				lights[i]->recalculateLSOrthoBasis(primarycamera->getForward(),
												   primarycamera->getPosition() + glm::vec3(0., 0.5, 0.), tempb, 8);
				delete[] tempb;
				lightuniformbuffertemp[i] = {
						lights[i]->getShadowPushConstantsPtr()->lightvpmatrices,
						lights[i]->getShadowPushConstantsPtr()->lspmatrix,
						lights[i]->getPosition(),
						lights[i]->getIntensity(),
						glm::vec3(0.0f, 0.0f, 0.0f),    //remember to change this if we need a forward vec
						lights[i]->getType(),
						lights[i]->getColor()
				};
				GraphicsHandler::VKHelperUpdateUniformBuffer(
						MAX_LIGHTS,
						sizeof(LightUniformBuffer),
						lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
						&lightuniformbuffertemp[0]);
			}
		}
	}

	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] & LIGHT_CHANGE_FLAG_BIT) {
		LightUniformBuffer lightuniformbuffertemp[MAX_LIGHTS];
		for (uint8_t i = 0; i < lights.size(); i++) {
			lightuniformbuffertemp[i] = {
					lights[i]->getShadowPushConstantsPtr()->lightvpmatrices,
					lights[i]->getShadowPushConstantsPtr()->lspmatrix,
					lights[i]->getPosition(),
					lights[i]->getIntensity(),
					glm::vec3(0.0f, 0.0f, 0.0f),    //remember to change this if we need a forward vec
					lights[i]->getType(),
					lights[i]->getColor()
			};
			GraphicsHandler::VKHelperUpdateUniformBuffer(
					MAX_LIGHTS,
					sizeof(LightUniformBuffer),
					lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
					&lightuniformbuffertemp[0]);
		}
		GraphicsHandler::vulkaninfo.primarygraphicspushconstants.numlights = lights.size();
		GraphicsHandler::vulkaninfo.oceanpushconstants.numlights = lights.size();
	}

	MeshUniformBuffer temp;
	for (auto& m: meshes) {
		temp = m->getUniformBufferData();
		GraphicsHandler::VKHelperUpdateUniformBuffer(
				1,
				sizeof(MeshUniformBuffer),
				m->getUniformBufferMemories()[GraphicsHandler::swapchainimageindex],
				reinterpret_cast<void*>(&temp));
	}

	GraphicsHandler::troubleshootingsstrm << "calculation time: " << (std::chrono::duration<double>(
			std::chrono::high_resolution_clock::now() - start).count());
	GraphicsHandler::troubleshootingsstrm << '\n' << (1.0f / (*physicshandler.getDtPtr())) << " fps"
										  << "\nswapchain image: " << GraphicsHandler::swapchainimageindex
										  << " frame in flight: " << GraphicsHandler::vulkaninfo.currentframeinflight
										  << "\nchangeflags: " << std::bitset<8>(
			GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]);

	VkPipelineStageFlags pipelinestageflags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VERBOSE_TIME_OPERATION(recordingthreads[0].join());

	collectSecondaryCommandBuffers();

	// TODO: move below to a GH submitandpresent func
	start = std::chrono::high_resolution_clock::now();
	VkSubmitInfo submitinfo {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			1,
			&GraphicsHandler::vulkaninfo.imageavailablesemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
			&pipelinestageflags,
			1,
			&GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
			1,
			&GraphicsHandler::vulkaninfo.renderfinishedsemaphores[GraphicsHandler::vulkaninfo.currentframeinflight]
	};
	vkResetFences(GraphicsHandler::vulkaninfo.logicaldevice, 1,
				  &GraphicsHandler::vulkaninfo.submitfinishedfences[GraphicsHandler::vulkaninfo.currentframeinflight]);
	vkQueueSubmit(GraphicsHandler::vulkaninfo.graphicsqueue,
				  1,
				  &submitinfo,
				  GraphicsHandler::vulkaninfo.submitfinishedfences[GraphicsHandler::vulkaninfo.currentframeinflight]);
	VkPresentInfoKHR presentinfo {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			1,
			&GraphicsHandler::vulkaninfo.renderfinishedsemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
			1,
			&GraphicsHandler::vulkaninfo.swapchain,
			&GraphicsHandler::swapchainimageindex,
			nullptr
	};
	vkQueuePresentKHR(GraphicsHandler::vulkaninfo.graphicsqueue, &presentinfo);

	// which one would be more efficient, modulus or ternary bounds check?
	GraphicsHandler::vulkaninfo.currentframeinflight =
			(GraphicsHandler::vulkaninfo.currentframeinflight + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool WanglingEngine::shouldClose () {
	return glfwWindowShouldClose(GraphicsHandler::vulkaninfo.window)
		   || glfwGetKey(GraphicsHandler::vulkaninfo.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

[[noreturn]] void WanglingEngine::threadedCmdBufRecord (WanglingEngine* self) {
	/* consider making immutable copy of all data being accessed (i.e. push constants & whatnot)
	 * channel allows communication between threads.
	 * consider "closures"
	 * another idea is a queue of function ptrs */
	std::unique_lock<std::mutex> ulock(submitfencemutex, std::defer_lock);
	ulock.lock();
	conditionvariable.wait(ulock, [] {return submitfenceavailable;});
	submitfenceavailable = false;
	vkWaitForFences(
			GraphicsHandler::vulkaninfo.logicaldevice,
			1,
			&GraphicsHandler::vulkaninfo.submitfinishedfences[GraphicsHandler::vulkaninfo.currentframeinflight],
			VK_FALSE,
			UINT64_MAX);
	ulock.unlock();
	submitfenceavailable = true;
	conditionvariable.notify_one();
	recordCommandBuffer(self, GraphicsHandler::vulkaninfo.currentframeinflight);
}