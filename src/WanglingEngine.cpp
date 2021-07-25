//
// Created by Daniel Paavola on 7/22/21.
//

#include "WanglingEngine.h"

WanglingEngine::WanglingEngine(){
	GraphicsHandler::VKInit();
	GraphicsHandler::VKInitPipelines(2, 1);
	primarycamera=new Camera(GraphicsHandler::vulkaninfo.window, GraphicsHandler::vulkaninfo.horizontalres, GraphicsHandler::vulkaninfo.verticalres);
	landscapemeshes=std::vector<Mesh*>();
	landscapemeshes.push_back(new Mesh("../resources/objs/lowpolybeach.obj", glm::vec3(0, 0, 0), &GraphicsHandler::vulkaninfo));
	dynamicshadowcastingmeshes=std::vector<Mesh*>();
	staticshadowcastingmeshes=std::vector<const Mesh*>();
	staticshadowcastingmeshes.push_back(new Mesh("../resources/objs/smoothhipolysuzanne.obj", glm::vec3(1, 1, 1), &GraphicsHandler::vulkaninfo));
	lights=std::vector<Light*>();
	lights.push_back(new Light(glm::vec3(100.0f, 100.0f, 0.0f),
							   glm::vec3(0.0f, 0.0f, 0.0f),
							   1.0f,
							   glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
							   32,
							   LIGHT_TYPE_SUN));
	GraphicsHandler::vulkaninfo.primarygraphicspushconstants={glm::mat4(1), glm::mat4(1), (uint32_t)lights.size()};
	troubleshootingtext=new Text("troubleshooting text",
							     glm::vec2(-1.0f, -1.0f),
							     glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
							     72.0f,
							     GraphicsHandler::vulkaninfo.horizontalres,
							     GraphicsHandler::vulkaninfo.verticalres);
	physicshandler=PhysicsHandler(landscapemeshes[0], primarycamera);
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) recordCommandBuffer(x);
	lasttime=std::chrono::high_resolution_clock::now();
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
						    &GraphicsHandler::vulkaninfo.primarygraphicspipeline.descriptorset[index],
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

	glfwPollEvents();

	//consider returning bool that signals whether camera has actually changed or not
	//if we had a whole system of change flags for camera, lights, and meshes, we could get a lot of efficiency with data sending and command recording
	primarycamera->takeInputs(GraphicsHandler::vulkaninfo.window);
	physicshandler.updateCameraPos();
	GraphicsHandler::vulkaninfo.primarygraphicspushconstants={
		primarycamera->getProjectionMatrix()*primarycamera->getViewMatrix(),
		glm::mat4(1),        //eventually we gotta move this to a uniform buffer so it can vary by mesh
		(uint32_t)lights.size()
	};

	//maybe move this to vulkaninfo struct? we don't have to, but it would be tidier, and might prove useful later???
	uint32_t imageindex=-1u;
	vkAcquireNextImageKHR(GraphicsHandler::vulkaninfo.logicaldevice,
				          GraphicsHandler::vulkaninfo.swapchain,
				          UINT64_MAX,
				          GraphicsHandler::vulkaninfo.imageavailablesemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
				          VK_NULL_HANDLE,
				          &imageindex);

	//could likely condense this whole process into like an UpdateUniformBuffer VKHelper
	if(GraphicsHandler::changeflags[imageindex]&LIGHT_CHANGE_FLAG_BIT){
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
		VkPhysicalDeviceProperties pdprops;
		vkGetPhysicalDeviceProperties(GraphicsHandler::vulkaninfo.physicaldevice, &pdprops);
		VkDeviceSize roundedupsize=sizeof(LightUniformBuffer);
		if(sizeof(LightUniformBuffer)%pdprops.limits.minUniformBufferOffsetAlignment!=0){
			roundedupsize=
					(1+floor((float)sizeof(LightUniformBuffer)/(float)pdprops.limits.minUniformBufferOffsetAlignment))*
					pdprops.limits.minUniformBufferOffsetAlignment;
		}
		void*datatemp;
		vkMapMemory(GraphicsHandler::vulkaninfo.logicaldevice,
		            GraphicsHandler::vulkaninfo.lightuniformbuffermemories[imageindex],
		            0,
		            roundedupsize*VULKAN_MAX_LIGHTS,
		            0,
		            &datatemp);
		char*dstscan=reinterpret_cast<char*>(datatemp), *srcscan=reinterpret_cast<char*>(&lightuniformbuffertemp[0]);
		for(uint32_t x=0;x<VULKAN_MAX_LIGHTS;x++){
			memcpy(reinterpret_cast<void*>(dstscan), reinterpret_cast<void*>(srcscan), sizeof(LightUniformBuffer));
			dstscan+=roundedupsize;
			srcscan+=sizeof(LightUniformBuffer);
		}
		vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice,
		              GraphicsHandler::vulkaninfo.lightuniformbuffermemories[imageindex]);
		GraphicsHandler::changeflags[imageindex]&=~LIGHT_CHANGE_FLAG_BIT;
	}

	if(GraphicsHandler::vulkaninfo.imageinflightfences[imageindex]!=VK_NULL_HANDLE){
		vkWaitForFences(GraphicsHandler::vulkaninfo.logicaldevice,
				        1,
				        &GraphicsHandler::vulkaninfo.imageinflightfences[imageindex],
				        VK_TRUE,
				        UINT64_MAX);
		std::stringstream sstrm;;
		sstrm<<(1.0f/std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-lasttime).count())<<" fps\n"
			 <<"swapchain image: "<<imageindex<<" frame in flight: "<<GraphicsHandler::vulkaninfo.currentframeinflight<<'\n'
			 <<"current tri: "<<std::hex<<physicshandler.getStandingTri()<<'\n'
			 <<"change flag bits: "<<std::dec<<std::bitset<8>(GraphicsHandler::changeflags[(imageindex+1)%GraphicsHandler::vulkaninfo.numswapchainimages]);
		troubleshootingtext->setMessage(sstrm.str(), imageindex);
		//can add check here once we figure out change flag system
		recordCommandBuffer(imageindex);
	}
	GraphicsHandler::vulkaninfo.imageinflightfences[imageindex]=GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight];
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
        &GraphicsHandler::vulkaninfo.commandbuffers[imageindex],
        1,
        &GraphicsHandler::vulkaninfo.renderfinishedsemaphores[GraphicsHandler::vulkaninfo.currentframeinflight]
	};
	//so this behavior isn't technically "double"-submitting, but it seems like the frame updates on a different timer than the main loop
	//look in swapchain config
	//note that this double-up isn't represented in the fps calculation, that's a different issue
	vkResetFences(GraphicsHandler::vulkaninfo.logicaldevice,
		          1,
		          &GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight]);
	vkQueueSubmit(GraphicsHandler::vulkaninfo.graphicsqueue,
			      1,
			      &submitinfo,
			      GraphicsHandler::vulkaninfo.frameinflightfences[GraphicsHandler::vulkaninfo.currentframeinflight]);
	VkPresentInfoKHR presentinfo{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&GraphicsHandler::vulkaninfo.renderfinishedsemaphores[GraphicsHandler::vulkaninfo.currentframeinflight],
		1,
		&GraphicsHandler::vulkaninfo.swapchain,
		&imageindex,
		nullptr
	};
	//okay so rate limiter is vkQueuePresentKHR, so if we want better fps, we should investigate optimal options/settings
	vkQueuePresentKHR(GraphicsHandler::vulkaninfo.graphicsqueue, &presentinfo);

	GraphicsHandler::vulkaninfo.currentframeinflight=(GraphicsHandler::vulkaninfo.currentframeinflight+1)%MAX_FRAMES_IN_FLIGHT;
	lasttime=std::chrono::high_resolution_clock::now();
}

bool WanglingEngine::shouldClose(){
	return glfwWindowShouldClose(GraphicsHandler::vulkaninfo.window)
		   ||glfwGetKey(GraphicsHandler::vulkaninfo.window, GLFW_KEY_ESCAPE)==GLFW_PRESS;
}