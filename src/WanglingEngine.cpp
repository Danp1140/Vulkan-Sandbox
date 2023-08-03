//
// Created by Daniel Paavola on 7/22/21.
//

#include "WanglingEngine.h"

std::mutex WanglingEngine::recordingmutex = std::mutex();
std::mutex WanglingEngine::scenedsmutex = std::mutex();
VkDescriptorSet* WanglingEngine::scenedescriptorsets = nullptr;

WanglingEngine::WanglingEngine () {
	TextureHandler::init();
	uint8_t nummeshes, numlights;
	countSceneObjects(WORKING_DIRECTORY "/resources/scenelayouts/rocktestlayout.json", &nummeshes, &numlights);
	// TODO: figure out best value for below arg
	GraphicsHandler::VKInit(200);
	GraphicsHandler::VKInitPipelines();
	loadScene(WORKING_DIRECTORY "/resources/scenelayouts/rocktestlayout.json");
	testterrain = new Terrain();
//	genScene();

	physicshandler = PhysicsHandler(primarycamera);

	// theres probably a better place for this
	GraphicsHandler::VKHelperInitTexture(&GraphicsHandler::vulkaninfo.ssrrdepthbuffer,
										 GraphicsHandler::vulkaninfo.swapchainextent.width,
										 GraphicsHandler::vulkaninfo.swapchainextent.height,
										 VK_FORMAT_D32_SFLOAT,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
										 TEXTURE_TYPE_SSRR_BUFFER,
										 VK_IMAGE_VIEW_TYPE_2D,
										 GraphicsHandler::genericsampler);
	GraphicsHandler::vulkaninfo.ssrrbuffers = new TextureInfo[GraphicsHandler::vulkaninfo.numswapchainimages];
	for (uint8_t scii = 0; scii < GraphicsHandler::vulkaninfo.numswapchainimages; scii++) {
//		VKHelperInitImage(&GraphicsHandler::vulkaninfo.ssrrbuffers[scii],
//						  GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height,
//						  VK_FORMAT_B8G8R8A8_SRGB,
//						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//						  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE);
		GraphicsHandler::VKHelperInitTexture(&GraphicsHandler::vulkaninfo.ssrrbuffers[scii],
											 GraphicsHandler::vulkaninfo.swapchainextent.width,
											 GraphicsHandler::vulkaninfo.swapchainextent.height,
											 VK_FORMAT_B8G8R8A8_SRGB,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
											 TEXTURE_TYPE_SSRR_BUFFER,
											 VK_IMAGE_VIEW_TYPE_2D,
											 GraphicsHandler::genericsampler); // could try messing with the sampler
	}

	// diff btwn normalmap and normaltex??????
	//TextureInfo* textemp = ocean->getNormalMapPtr();
	//texturehandler.generateOceanTextures(&textemp, 1);
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

//	meshes.push_back(new Mesh(WORKING_DIRECTORY "/resources/objs/fuckingcube.obj", glm::vec3(3., 1., -2.)));
	meshes.push_back(new Mesh(WORKING_DIRECTORY "/resources/objs/smoothhipolysuzanne.obj",
							  glm::vec3(0., -1., 5.),
							  glm::vec3(1.),
							  glm::quat(sin(1.57), 0., 0., cos(1.57)), 2048u));
	TextureHandler::generateTextures({*meshes.back()->getDiffuseTexturePtr()}, TextureHandler::colorfulMarbleTexGenSet);

//	TextureHandler::generateTextures({*meshes[0]->getDiffuseTexturePtr()}, TextureHandler::gridTexGenSet);
//	meshes[0]->rewriteTextureDescriptorSets();

	initSceneData();

	initTroubleshootingLines();

//	 updateTexMonDescriptorSets(*lights[0]->getShadowmapPtr());
//	updateTexMonDescriptorSets(GraphicsHandler::vulkaninfo.ssrrbuffers[0]);
	updateTexMonDescriptorSets(GraphicsHandler::vulkaninfo.ssrrdepthbuffer);
//	updateTexMonDescriptorSets(GraphicsHandler::vulkaninfo.depthbuffer);

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
		enqueueRecordingTasks();
		for (uint8_t i = 0; i < NUM_RECORDING_THREADS; i++) {
			recordingthreads[i] = std::thread(processRecordingTasks,
											  &recordingtasks,
											  &secondarybuffers,
											  x,
											  i,
											  GraphicsHandler::vulkaninfo.logicaldevice);
		}
		for (uint8_t i = 0; i < NUM_RECORDING_THREADS; i++) {
			recordingthreads[i].join();
		}
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
	meshes[0]->getDiffuseTexturePtr()->setUVScale(glm::vec2(.1f, .1f));
	meshes[0]->rewriteTextureDescriptorSets();

	ocean = new Ocean(glm::vec3(-10., -2., -10.), glm::vec2(20.), meshes[0]);
//	grass = new ParticleSystem<GrassParticle>(GRASS, 100, ENFORCED_UNIFORM_ON_MESH, {meshes[0]});
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
//	std::vector<glm::vec3> controlpoints {
//			glm::vec3(11., 0., 11.),
//			glm::vec3(10., 0., -8.),
//			glm::vec3(4., 0., -6.),
//			glm::vec3(4., 0., 4.),
//			glm::vec3(-4., 0., 6.),
//			glm::vec3(-6., 0., 4.),
//			glm::vec3(-10., 0., -4),
//			glm::vec3(-11., 0., 4.)
//	};
//	glm::vec3 splinePos;
//	for (uint8_t p1 = 1; p1 + 2 < controlpoints.size(); p1++) {
//		for (uint8_t x = 0; x < 25; x++) {
//			splinePos = PhysicsHandler::catmullRomSplineAtMatrified(
//					controlpoints,
//					p1,
//					0.5,
//					float_t(x) / 24.f);
//			troubleshootinglinespoints.push_back({
//														 splinePos,
//														 glm::vec3(0., 0., 0.),
//														 glm::vec2(0., 0.)
//												 });
//			troubleshootinglinespoints.push_back({
//														 splinePos,
//														 glm::vec3(0., 0., 0.),
//														 glm::vec2(0., 0.)
//												 });
//
//		}
//	}
//	troubleshootinglinespoints.erase(troubleshootinglinespoints.begin());
//	troubleshootinglinespoints.erase(troubleshootinglinespoints.end() - 1);

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

void WanglingEngine::recordTexMonCommandBuffers (cbRecData data, VkCommandBuffer& cb) {
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
			GraphicsHandler::vulkaninfo.texmongraphicspipeline.pipeline);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.texmongraphicspipeline.pipelinelayout,
			0,
			1, &data.descriptorset,
			0, nullptr);
	vkCmdDraw(cb, 6, 1, 0, 0);
	vkEndCommandBuffer(cb);
}

void WanglingEngine::recordTroubleshootingLinesCommandBuffers (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cbii {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			data.renderpass,
			0,
			data.framebuffer,
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
			cb,
			&cbbi);
	vkCmdBindPipeline(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipeline);
	vkCmdPushConstants(
			cb,
			data.pipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4),
			&GraphicsHandler::vulkaninfo.grasspushconstants);        // this is odd
	VkDeviceSize offsettemp = 0u;
	vkCmdBindVertexBuffers(
			cb,
			0,
			1,
			&data.vertexbuffer,
			&offsettemp);
	vkCmdDraw(
			cb,
			254,        // so is this
			1,
			0,
			0);
	vkEndCommandBuffer(cb);
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
	self->ocean->recordCompute(fifindex);
	self->ocean->recordDraw(fifindex, GraphicsHandler::swapchainimageindex, self->scenedescriptorsets);
	self->grass->recordDraw(fifindex, GraphicsHandler::swapchainimageindex, self->scenedescriptorsets);

	vkResetCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 0);

	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], &cmdbufferbegininfo);

	if (GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] | LIGHT_CHANGE_FLAG_BIT) {
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

//	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex], 1,
//						 &self->grass->getCommandBuffers()[fifindex]);

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

//	vkCmdExecuteCommands(GraphicsHandler::vulkaninfo.commandbuffers[fifindex],
//						 1, &self->ocean->getCommandBuffers()[fifindex]);

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

// TODO: figure out which buffer recording setup structs can be made static const for efficiency
void WanglingEngine::enqueueRecordingTasks () {
	cbRecData tempdata {VK_NULL_HANDLE,
						VK_NULL_HANDLE,
						VK_NULL_HANDLE,
						VK_NULL_HANDLE,
						{},
						nullptr,
						nullptr,
						VK_NULL_HANDLE, VK_NULL_HANDLE, 0u};
	// hey so im not even certain that i have to sync ds access, maybe just alloc/free

	tempdata.pipeline = GraphicsHandler::vulkaninfo.terraingencomputepipeline;
	tempdata.descriptorset = testterrain->getDS(GraphicsHandler::swapchainimageindex);
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Terrain::recordCompute(tempdata, c);}));

	tempdata.pipeline = GraphicsHandler::vulkaninfo.oceancomputepipeline;
	tempdata.descriptorset = ocean->getComputeDescriptorSets()[GraphicsHandler::swapchainimageindex]; // double-check that this is correct idx
	tempdata.pushconstantdata = reinterpret_cast<void*>(physicshandler.getTPtr());
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Ocean::recordCompute(tempdata, c);}));

	// do we need those ds mutexes???
	// try removing them from draws once we have both mesh draws in and are stress testing
	// TODO: check for light change flag
	tempdata.scenedescriptorset = scenedescriptorsets[GraphicsHandler::swapchainimageindex];
	tempdata.pipeline = GraphicsHandler::vulkaninfo.shadowmapgraphicspipeline;
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
			tempdata.descriptorset = m->getDescriptorSets()[GraphicsHandler::swapchainimageindex];
			tempdata.dsmutex = m->getDSMutexPtr();
			tempdata.vertexbuffer = m->getVertexBuffer();
			tempdata.indexbuffer = m->getIndexBuffer();
			tempdata.numtris = m->getTrisPtr()->size();
			recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Mesh::recordShadowDraw(tempdata, c);}));
		}
		// could we put this in a constructor or smth?
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

	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
										  nullptr,
										  GraphicsHandler::vulkaninfo.primaryrenderpass,
										  GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex],
										  {{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
										  2,
										  &GraphicsHandler::vulkaninfo.primaryclears[0]
								  }));
	tempdata.renderpass = GraphicsHandler::vulkaninfo.primaryrenderpass;        // should maybe make this a different renderpass for skybox
	tempdata.framebuffer = GraphicsHandler::vulkaninfo.primaryframebuffers[GraphicsHandler::swapchainimageindex];

	tempdata.descriptorset = skyboxdescriptorsets[GraphicsHandler::swapchainimageindex]; // TODO: figure out whether we want one ds per sci or fif
	tempdata.pipeline = GraphicsHandler::vulkaninfo.skyboxgraphicspipeline;
	tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.skyboxpushconstants);
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {recordSkyboxCommandBuffers(tempdata, c);}));

	tempdata.pipeline = GraphicsHandler::vulkaninfo.primarygraphicspipeline;
	tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.primarygraphicspushconstants);
	for (auto& m: meshes) {
		tempdata.descriptorset = m->getDescriptorSets()[GraphicsHandler::swapchainimageindex];
		tempdata.dsmutex = m->getDSMutexPtr();
		tempdata.vertexbuffer = m->getVertexBuffer();
		tempdata.indexbuffer = m->getIndexBuffer();
		tempdata.numtris = m->getTrisPtr()->size();
		recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Mesh::recordDraw(tempdata, c);}));
	}

	tempdata.pipeline = GraphicsHandler::vulkaninfo.linegraphicspipeline;
	tempdata.vertexbuffer = troubleshootinglinesvertexbuffer;
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {
		recordTroubleshootingLinesCommandBuffers(tempdata,
												 c);
	}));

	tempdata.pipeline = GraphicsHandler::vulkaninfo.voxeltroubleshootingpipeline;
	tempdata.descriptorset = testterrain->getDS(GraphicsHandler::swapchainimageindex);
	tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.terrainpushconstants);
	tempdata.numtris = testterrain->getNumLeaves() * 2u;
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Terrain::recordTroubleshootDraw(tempdata, c);}));

	// cpy op should take place here, only one texture needs to exist, probably in GH, cpy op could be in secondary buff, or we add a new type of cbRecTask
	// we put it down below temporarily so we didnt need another renderpass for 2d stuff, but in the future we should do that

	tempdata.descriptorset = texmondescriptorsets[GraphicsHandler::vulkaninfo.currentframeinflight];
	tempdata.pipeline = GraphicsHandler::vulkaninfo.texmongraphicspipeline;
//	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {recordTexMonCommandBuffers(tempdata, c);}));

	tempdata.descriptorset = troubleshootingtext->getDescriptorSets()[GraphicsHandler::vulkaninfo.currentframeinflight];
	tempdata.pipeline = GraphicsHandler::vulkaninfo.textgraphicspipeline;
	tempdata.pushconstantdata = reinterpret_cast<void*>(troubleshootingtext->getPushConstantData());
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Text::recordCommandBuffer(tempdata, c);}));

	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
										  nullptr,
										  VK_NULL_HANDLE,
										  GraphicsHandler::vulkaninfo.ssrrframebuffers[GraphicsHandler::swapchainimageindex],
										  {{0, 0}, {0, 0}},
										  0, nullptr
								  }));


	// if this works, move and change to static

	// feels a bit hamfisted to have so many membars, well do it like this for now, but see if there is a more elegant way later
	// can combine these into one dependency rec task w/ 2 membars
	VkImageMemoryBarrier2KHR* ssrrcolorpreimb = new VkImageMemoryBarrier2KHR {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			GraphicsHandler::vulkaninfo.ssrrbuffers[GraphicsHandler::swapchainimageindex].image,
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
										  nullptr,
										  0,
										  0, nullptr,
										  0, nullptr,
										  1, ssrrcolorpreimb
								  }));
	VkImageMemoryBarrier2KHR* ssrrdepthpreimb = new VkImageMemoryBarrier2KHR {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			GraphicsHandler::vulkaninfo.ssrrdepthbuffer.image,
			{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	};
	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
										  nullptr,
										  0,
										  0, nullptr,
										  0, nullptr,
										  1, ssrrdepthpreimb
								  }));

	VkImageSubresourceLayers subreclayers {
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			0,
			1
	};
	VkImageCopy2 imgcpy {
			VK_STRUCTURE_TYPE_IMAGE_COPY_2,
			nullptr,
			subreclayers, {0, 0, 0},
			subreclayers, {0, 0, 0},
			{GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height, 1}
	};
	VkCopyImageInfo2 cpyinfo {
			VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
			nullptr,
			GraphicsHandler::vulkaninfo.swapchainimages[GraphicsHandler::swapchainimageindex],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			GraphicsHandler::vulkaninfo.ssrrbuffers[GraphicsHandler::swapchainimageindex].image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imgcpy
	};
	recordingtasks.push(cbRecTask([tempdata, cpyinfo] (VkCommandBuffer& c) {
		GraphicsHandler::recordImgCpy(tempdata,
									  cpyinfo,
									  c);
	}));

	VkImageSubresourceLayers depthsubreclayers {
			VK_IMAGE_ASPECT_DEPTH_BIT,
			0,
			0,
			1
	};
	VkImageCopy2 depthimgcpy {
			VK_STRUCTURE_TYPE_IMAGE_COPY_2,
			nullptr,
			depthsubreclayers, {0, 0, 0},
			depthsubreclayers, {0, 0, 0},
			{GraphicsHandler::vulkaninfo.swapchainextent.width, GraphicsHandler::vulkaninfo.swapchainextent.height, 1}
	};
	VkCopyImageInfo2 depthcpyinfo {
			VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
			nullptr,
			GraphicsHandler::vulkaninfo.depthbuffer.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // i'm confused here, check renderpass, may need layout transition
			GraphicsHandler::vulkaninfo.ssrrdepthbuffer.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &depthimgcpy
	};

	recordingtasks.push(cbRecTask([tempdata, depthcpyinfo] (VkCommandBuffer& c) {
		GraphicsHandler::recordImgCpy(tempdata,
									  depthcpyinfo,
									  c);
	}));

	// lmao why is this a pointer
	// TODO: possible memory leak lmaoooooo
	VkImageMemoryBarrier2KHR* ssrrcolorimb = new VkImageMemoryBarrier2KHR {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			GraphicsHandler::vulkaninfo.ssrrbuffers[GraphicsHandler::swapchainimageindex].image,
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
										  nullptr,
										  0,
										  0, nullptr,
										  0, nullptr,
										  1, ssrrcolorimb
								  }));

	VkImageMemoryBarrier2KHR* ssrrdepthimb = new VkImageMemoryBarrier2KHR {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			GraphicsHandler::vulkaninfo.ssrrdepthbuffer.image,
			{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	};
	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
										  nullptr,
										  0,
										  0, nullptr,
										  0, nullptr,
										  1, ssrrdepthimb
								  }));


	// random thought: could we cut it down to one ssrr buffer that gets reused for every frame??

	recordingtasks.push(cbRecTask({
										  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
										  nullptr,
										  GraphicsHandler::vulkaninfo.ssrrrenderpass,
										  GraphicsHandler::vulkaninfo.ssrrframebuffers[GraphicsHandler::swapchainimageindex],
										  {{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
										  2,
										  &GraphicsHandler::vulkaninfo.primaryclears[0]
								  }));

	tempdata.renderpass = GraphicsHandler::vulkaninfo.ssrrrenderpass;
	tempdata.framebuffer = GraphicsHandler::vulkaninfo.ssrrframebuffers[GraphicsHandler::swapchainimageindex];
	tempdata.pipeline = GraphicsHandler::vulkaninfo.oceangraphicspipeline;
	tempdata.pushconstantdata = reinterpret_cast<void*>(&GraphicsHandler::vulkaninfo.oceanpushconstants);
	tempdata.descriptorset = ocean->getDescriptorSets()[GraphicsHandler::swapchainimageindex]; // check that this is the correct index
	tempdata.vertexbuffer = ocean->getVertexBuffer();
	tempdata.indexbuffer = ocean->getIndexBuffer();
	tempdata.numtris = ocean->getTris().size();
	recordingtasks.push(cbRecTask([tempdata] (VkCommandBuffer& c) {Ocean::recordDraw(tempdata, c);}));
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
	vkResetCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
						 0); // is this call neccesary???

	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
						 &cmdbufferbegininfo);
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
				if (inrp) {
					vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
					inrp = false;
				}

				// WLYL: you were investigating this "dependency" barrier system, and learning abt pipeline barrier to facilitate copying the screen buffer
				// when troubleshooting this, try monitoring the memory in texmon
				// perhaps just try running a cpy lol

				VkImageMemoryBarrier imb {
						// TODO: sequester this shit to a function
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].sType,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].pNext,
						(VkAccessFlags)secondarybuffers.front().data.di.pImageMemoryBarriers[0].srcAccessMask,
						(VkAccessFlags)secondarybuffers.front().data.di.pImageMemoryBarriers[0].dstAccessMask,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].oldLayout,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].newLayout,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].srcQueueFamilyIndex,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].dstQueueFamilyIndex,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].image,
						secondarybuffers.front().data.di.pImageMemoryBarriers[0].subresourceRange
				};
				vkCmdPipelineBarrier(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight],
									 secondarybuffers.front().data.di.pImageMemoryBarriers[0].srcStageMask,
									 secondarybuffers.front().data.di.pImageMemoryBarriers[0].dstStageMask,
									 secondarybuffers.front().data.di.dependencyFlags,
									 0, nullptr,
									 0, nullptr,
									 1, &imb);
				delete secondarybuffers.front().data.di.pImageMemoryBarriers;
			}
		}
		secondarybuffers.pop();
	}
	if (inrp)
		vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	vkEndCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::vulkaninfo.currentframeinflight]);
	GraphicsHandler::changeflags[GraphicsHandler::vulkaninfo.currentframeinflight] = NO_CHANGE_FLAG_BIT;
}

void WanglingEngine::draw () {
	std::chrono::time_point<std::chrono::high_resolution_clock> start;

	vkAcquireNextImageKHR(
			GraphicsHandler::vulkaninfo.logicaldevice,
			GraphicsHandler::vulkaninfo.swapchain,
			UINT64_MAX,
			GraphicsHandler::vulkaninfo.imageavailablesemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
			VK_NULL_HANDLE,
			&GraphicsHandler::swapchainimageindex);

	vkWaitForFences(
			GraphicsHandler::vulkaninfo.logicaldevice,
			1, &GraphicsHandler::vulkaninfo.submitfinishedfences[GraphicsHandler::vulkaninfo.currentframeinflight],
			VK_FALSE,
			UINT64_MAX);
	troubleshootingtext->setMessage(GraphicsHandler::troubleshootingsstrm.str(),
									GraphicsHandler::swapchainimageindex);
	GraphicsHandler::troubleshootingsstrm = std::stringstream(std::string());


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
					glm::vec3(0.0f, 0.0f, 0.0f),    //remember to change this if we need a forward vec
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
										  << " frame in flight: "
										  << GraphicsHandler::vulkaninfo.currentframeinflight
										  << "\nchangeflags: " << std::bitset<8>(
			GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]);

	VkPipelineStageFlags pipelinestageflags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	enqueueRecordingTasks();

	for (uint8_t x = 0; x < NUM_RECORDING_THREADS; x++) {
		recordingthreads[x] = std::thread(processRecordingTasks,
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

	// TODO: move below to a GH submitandpresent func
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