//
// Created by Daniel Paavola on 7/22/21.
//

#include "WanglingEngine.h"

WanglingEngine::WanglingEngine(){
	GraphicsHandler::VKInit();
	GraphicsHandler::VKInitPipelines(2, 1);
	primarycamera=new Camera(GraphicsHandler::vulkaninfo.window, GraphicsHandler::vulkaninfo.horizontalres, GraphicsHandler::vulkaninfo.verticalres);
	texturehandler=TextureHandler();
	landscapemeshes=std::vector<Mesh*>();
	landscapemeshes.push_back(new Mesh("../resources/objs/lowpolybeach.obj", glm::vec3(0, 0, 0), &GraphicsHandler::vulkaninfo));
	glm::vec4 gradienttemp[2]={glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(1.0, 1.0, 0.5, 1.0)};
	//would like a cleaner method of doing this, especially handling descriptor set updates
	texturehandler.generateStaticSandTextures(&gradienttemp[0],
											   1024,
											   0,
											   landscapemeshes.back()->getDiffuseTexturePtr(),
											   landscapemeshes.back()->getNormalTexturePtr());
	landscapemeshes.back()->rewriteAllDescriptorSets();
	dynamicshadowcastingmeshes=std::vector<Mesh*>();
//	dynamicshadowcastingmeshes.push_back(new Mesh("../resources/objs/smoothhipolysuzanne.obj", glm::vec3(1, 1, 1), &GraphicsHandler::vulkaninfo));
//	texturehandler.generateMonotoneTextures(glm::vec4(1.0, 0.0, 0.0, 1.0),
//	                                         1024,
//	                                         0,
//	                                         dynamicshadowcastingmeshes.back()->getDiffuseTexturePtr(),
//	                                         dynamicshadowcastingmeshes.back()->getNormalTexturePtr());
//	dynamicshadowcastingmeshes.back()->rewriteAllDescriptorSets();
	staticshadowcastingmeshes=std::vector<const Mesh*>();
	//TODO: actually send over the model matrix lol
	physicshandler=PhysicsHandler(landscapemeshes[0], primarycamera);
	lights=std::vector<Light*>();
	lights.push_back(new Light(glm::vec3(100.0f, 100.0f, 100.0f),
							   glm::vec3(0.0f, 0.0f, 0.0f),
							   1.0f,
							   glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
							   32,
							   LIGHT_TYPE_SUN));
	GraphicsHandler::vulkaninfo.primarygraphicspushconstants={glm::mat4(1), glm::mat4(1), glm::vec2(0), (uint32_t)lights.size()};
	ocean=new Ocean(glm::vec3(0.0, -0.5, 10.0), glm::vec2(10.0, 10.0), landscapemeshes[0]);
	texturehandler.generateOceanTextures(512, 0, ocean->getHeightMapPtr(), ocean->getNormalMapPtr());
	ocean->rewriteAllDescriptorSets();
	troubleshootingtext=new Text("troubleshooting text",
							     glm::vec2(-1.0f, -1.0f),
							     glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
							     72.0f,
							     GraphicsHandler::vulkaninfo.horizontalres,
							     GraphicsHandler::vulkaninfo.verticalres);
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) recordCommandBuffer(x);
}

void WanglingEngine::recordCommandBuffer(uint32_t index){
//	vkResetCommandBuffer(vulkaninfo.commandbuffers[x], 0);
	VkCommandBufferBeginInfo cmdbufferbegininfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		0,
		nullptr
	};

	vkBeginCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[index], &cmdbufferbegininfo);
	VkRenderPassBeginInfo renderpassbegininfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.primaryrenderpass,
		GraphicsHandler::vulkaninfo.framebuffers[index],
		{{0, 0}, GraphicsHandler::vulkaninfo.swapchainextent},
		2,
		&GraphicsHandler::vulkaninfo.clears[0]
	};
	vkCmdBeginRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[index], &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(GraphicsHandler::vulkaninfo.commandbuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipeline);
	vkCmdPushConstants(GraphicsHandler::vulkaninfo.commandbuffers[index],
					   GraphicsHandler::vulkaninfo.skyboxgraphicspipeline.pipelinelayout,
					   VK_SHADER_STAGE_FRAGMENT_BIT,
					   0,
					   sizeof(SkyboxPushConstants),
					   &GraphicsHandler::vulkaninfo.skyboxpushconstants);
	vkCmdDraw(GraphicsHandler::vulkaninfo.commandbuffers[index], 6, 1, 0, 0);

	vkCmdBindPipeline(GraphicsHandler::vulkaninfo.commandbuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipeline);
	vkCmdPushConstants(GraphicsHandler::vulkaninfo.commandbuffers[index],
					   GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
					   VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
					   0,
					   sizeof(PrimaryGraphicsPushConstants),
					   (void*)(&GraphicsHandler::vulkaninfo.primarygraphicspushconstants));
	vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
						    VK_PIPELINE_BIND_POINT_GRAPHICS,
						    GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
						    0,
						    1,
						    &GraphicsHandler::scenedescriptorsets[index],
						    0,
						    nullptr);
	VkDeviceSize offsettemp=0u;
	for(auto&m:landscapemeshes){
		vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
						        VK_PIPELINE_BIND_POINT_GRAPHICS,
						        GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
						        1,
						        1,
						        &m->getDescriptorSets()[index],
						        0,
						        nullptr);
		vkCmdBindVertexBuffers(GraphicsHandler::vulkaninfo.commandbuffers[index], 0, 1, m->getVertexBuffer(), &offsettemp);
		vkCmdBindIndexBuffer(GraphicsHandler::vulkaninfo.commandbuffers[index], *m->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(GraphicsHandler::vulkaninfo.commandbuffers[index], m->getTris().size()*3, 1, 0 ,0, 0);
	}
	for(auto&m:staticshadowcastingmeshes){
		vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
		                        1,
		                        1,
		                        &m->getDescriptorSets()[index],
		                        0,
		                        nullptr);
		vkCmdBindVertexBuffers(GraphicsHandler::vulkaninfo.commandbuffers[index], 0, 1, m->getVertexBuffer(), &offsettemp);
		vkCmdBindIndexBuffer(GraphicsHandler::vulkaninfo.commandbuffers[index], *m->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(GraphicsHandler::vulkaninfo.commandbuffers[index], m->getTris().size()*3, 1, 0 ,0, 0);
	}
	for(auto&m:dynamicshadowcastingmeshes){
		vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
		                        1,
		                        1,
		                        &m->getDescriptorSets()[index],
		                        0,
		                        nullptr);
		vkCmdBindVertexBuffers(GraphicsHandler::vulkaninfo.commandbuffers[index], 0, 1, m->getVertexBuffer(), &offsettemp);
		vkCmdBindIndexBuffer(GraphicsHandler::vulkaninfo.commandbuffers[index], *m->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(GraphicsHandler::vulkaninfo.commandbuffers[index], m->getTris().size()*3, 1, 0 ,0, 0);
	}
////	vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
////						    VK_PIPELINE_BIND_POINT_GRAPHICS,
////						    GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
////						    1,
////						    1,
////						    &ocean->getDescriptorSets()[index],
////						    0,
////						    nullptr);
	vkCmdBindPipeline(GraphicsHandler::vulkaninfo.commandbuffers[index],
				      VK_PIPELINE_BIND_POINT_GRAPHICS,
				      GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipeline);
	vkCmdPushConstants(GraphicsHandler::vulkaninfo.commandbuffers[index],
					   GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipelinelayout,
					   VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
					   0,
					   sizeof(OceanPushConstants),
					   &GraphicsHandler::vulkaninfo.oceanpushconstants);
	vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
						    VK_PIPELINE_BIND_POINT_GRAPHICS,
						    GraphicsHandler::vulkaninfo.oceangraphicspipeline.pipelinelayout,
						    0,
						    1,
						    ocean->getDescriptorSets(),
						    0,
						    nullptr);
	vkCmdBindVertexBuffers(GraphicsHandler::vulkaninfo.commandbuffers[index],
	                       0,
	                       1,
	                       ocean->getVertexBuffer(),
	                       &offsettemp);
	vkCmdBindIndexBuffer(GraphicsHandler::vulkaninfo.commandbuffers[index],
	                     *(ocean->getIndexBuffer()),
	                     0,
	                     VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(GraphicsHandler::vulkaninfo.commandbuffers[index],
	                 ocean->getTris().size()*3,
	                 1,
	                 0,
	                 0,
	                 0);
	vkCmdBindPipeline(GraphicsHandler::vulkaninfo.commandbuffers[index],
		              VK_PIPELINE_BIND_POINT_GRAPHICS,
		              GraphicsHandler::vulkaninfo.textgraphicspipeline.pipeline);
	vkCmdPushConstants(GraphicsHandler::vulkaninfo.commandbuffers[index],
					   GraphicsHandler::vulkaninfo.textgraphicspipeline.pipelinelayout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(TextPushConstants),
					   (void*)troubleshootingtext->getPushConstantsPtr());
	vkCmdBindDescriptorSets(GraphicsHandler::vulkaninfo.commandbuffers[index],
						    VK_PIPELINE_BIND_POINT_GRAPHICS,
						    GraphicsHandler::vulkaninfo.textgraphicspipeline.pipelinelayout,
						    0,
						    1,
						    &troubleshootingtext->getDescriptorSetsPtr()[index],
						    0,
						    nullptr);
	vkCmdDraw(GraphicsHandler::vulkaninfo.commandbuffers[index], 6, 1, 0, 0);

	vkCmdEndRenderPass(GraphicsHandler::vulkaninfo.commandbuffers[index]);
	vkEndCommandBuffer(GraphicsHandler::vulkaninfo.commandbuffers[index]);
}

void WanglingEngine::draw(){
	vkWaitForFences(GraphicsHandler::vulkaninfo.logicaldevice,
				    1,
				    &GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight],
				    VK_TRUE,
				    UINT64_MAX);
	vkAcquireNextImageKHR(GraphicsHandler::vulkaninfo.logicaldevice,
	                      GraphicsHandler::vulkaninfo.swapchain,
	                      UINT64_MAX,
	                      GraphicsHandler::vulkaninfo.imageavailablesemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
	                      VK_NULL_HANDLE,
	                      &GraphicsHandler::swapchainimageindex);
	glfwPollEvents();

	auto start=std::chrono::high_resolution_clock::now();
	primarycamera->takeInputs(GraphicsHandler::vulkaninfo.window);
	physicshandler.update();
	if(GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]&CAMERA_POSITION_CHANGE_FLAG_BIT
		||GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]&CAMERA_LOOK_CHANGE_FLAG_BIT){
		GraphicsHandler::vulkaninfo.primarygraphicspushconstants={
				primarycamera->getProjectionMatrix()*primarycamera->getViewMatrix(),
				glm::mat4(1),        //eventually we gotta move this to a uniform buffer so it can vary by mesh
				physicshandler.getStandingUV(),
				(uint32_t)lights.size()
		};
		GraphicsHandler::vulkaninfo.oceanpushconstants={primarycamera->getProjectionMatrix()*primarycamera->getViewMatrix()};
	}
	if(GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]&CAMERA_LOOK_CHANGE_FLAG_BIT
		||GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]&LIGHT_CHANGE_FLAG_BIT){
		//likely some efficiency to be had here, esp w/ crosses
		GraphicsHandler::vulkaninfo.skyboxpushconstants={
				glm::normalize(primarycamera->getForward())
					*(1.0f/tan(primarycamera->getFovy()/2.0)),
				glm::normalize(glm::cross(primarycamera->getForward(), glm::vec3(0.0f, 1.0f, 0.0f)))
					*2.0f/(float)GraphicsHandler::vulkaninfo.horizontalres,
				glm::vec3(0.0f, 0.0f, 0.0f),
				lights[0]->getPosition()
		};
		GraphicsHandler::vulkaninfo.skyboxpushconstants.dy=-glm::normalize(
				glm::cross( GraphicsHandler::vulkaninfo.skyboxpushconstants.dx,
				primarycamera->getForward()))
				*2.0/(float)GraphicsHandler::vulkaninfo.verticalres;
	}
//	lights[0]->setPosition(glm::vec3(sin(glfwGetTime()), 0.5, cos(glfwGetTime())));
	if(GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]&LIGHT_CHANGE_FLAG_BIT){
		LightUniformBuffer lightuniformbuffertemp[VULKAN_MAX_LIGHTS];
		for(uint32_t x=0;x<lights.size();x++){
			lightuniformbuffertemp[x]={
					lights[x]->getPosition(),
					lights[x]->getIntensity(),
					glm::vec3(0.0f, 0.0f, 0.0f),
					lights[x]->getType(),
					lights[x]->getColor()
			};
		}
		GraphicsHandler::VKHelperUpdateUniformBuffer(VULKAN_MAX_LIGHTS,
											         sizeof(LightUniformBuffer),
											         GraphicsHandler::vulkaninfo.lightuniformbuffermemories[GraphicsHandler::swapchainimageindex],
											         &lightuniformbuffertemp[0]);
	}
	GraphicsHandler::troubleshootingsstrm<<"calculation time: "<<(std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count())<<'\n';

	if(GraphicsHandler::vulkaninfo.imageinflightfences[GraphicsHandler::swapchainimageindex]!=VK_NULL_HANDLE){
		start=std::chrono::high_resolution_clock::now();
		vkWaitForFences(GraphicsHandler::vulkaninfo.logicaldevice,
				        1,
				        &GraphicsHandler::vulkaninfo.imageinflightfences[GraphicsHandler::swapchainimageindex],
				        VK_TRUE,
				        UINT64_MAX);
		GraphicsHandler::troubleshootingsstrm<<"record fence wait time: "<<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count()<<'\n'
											 <<(1.0f/(*physicshandler.getDtPtr()))<<" fps"
		                                     <<"\nswapchain image: "<<GraphicsHandler::swapchainimageindex<<" frame in flight: "<<GraphicsHandler::vulkaninfo.currentframeinflight
		                                     <<"\ncurrent tri: "<<std::hex<<physicshandler.getStandingTri()<<std::dec
		                                     <<"\nchangeflags: "<<std::bitset<8>(GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]);
		troubleshootingtext->setMessage(GraphicsHandler::troubleshootingsstrm.str(), GraphicsHandler::swapchainimageindex);
		start=std::chrono::high_resolution_clock::now();
		recordCommandBuffer(GraphicsHandler::swapchainimageindex);
		GraphicsHandler::troubleshootingsstrm.str(std::string());
		GraphicsHandler::troubleshootingsstrm<<"recording time: "<<(std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count())<<'\n';
		GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]=NO_CHANGE_FLAG_BIT;
	}
	GraphicsHandler::vulkaninfo.imageinflightfences[GraphicsHandler::swapchainimageindex]=GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight];
	VkPipelineStageFlags pipelinestageflags=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//slight efficiency to be had by holding on to submitinfo and just swapping out parts that change
	//same goes for presentinfo down below
	VkSubmitInfo submitinfo{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		1,
		&GraphicsHandler::vulkaninfo.imageavailablesemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
		&pipelinestageflags,
		1,
        &GraphicsHandler::vulkaninfo.commandbuffers[GraphicsHandler::swapchainimageindex],
        1,
        &GraphicsHandler::vulkaninfo.renderfinishedsemaphores[GraphicsHandler::vulkaninfo.currentframeinflight]
	};
	vkResetFences(GraphicsHandler::vulkaninfo.logicaldevice,
		          1,
		          &GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight]);
	//okay its becoming clear to me that i don't quite understand the interactions between how this loop runs and how the image index progresses
	start=std::chrono::high_resolution_clock::now();
	vkQueueSubmit(GraphicsHandler::vulkaninfo.graphicsqueue,
			      1,
			      &submitinfo,
			      GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight]);
	GraphicsHandler::troubleshootingsstrm<<"submission time: "<<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count()<<'\n';
	VkPresentInfoKHR presentinfo{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&GraphicsHandler::vulkaninfo.renderfinishedsemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
		1,
		&GraphicsHandler::vulkaninfo.swapchain,
		&GraphicsHandler::swapchainimageindex,
		nullptr
	};
	//okay so rate limiter is vkQueuePresentKHR, so if we want better fps, we should investigate optimal options/settings
	start=std::chrono::high_resolution_clock::now();
	vkQueuePresentKHR(GraphicsHandler::vulkaninfo.graphicsqueue, &presentinfo);
	GraphicsHandler::troubleshootingsstrm<<"presentation time: "<<std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-start).count()<<'\n';

	GraphicsHandler::vulkaninfo.currentframeinflight=(GraphicsHandler::vulkaninfo.currentframeinflight+1)%MAX_FRAMES_IN_FLIGHT;
}

bool WanglingEngine::shouldClose(){
	return glfwWindowShouldClose(GraphicsHandler::vulkaninfo.window)
		   ||glfwGetKey(GraphicsHandler::vulkaninfo.window, GLFW_KEY_ESCAPE)==GLFW_PRESS;
}