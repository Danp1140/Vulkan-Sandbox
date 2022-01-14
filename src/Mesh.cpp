//
// Created by Daniel Paavola on 5/30/21.
//

#include "Mesh.h"

VulkanInfo*Mesh::vulkaninfo{};

Mesh::Mesh(VulkanInfo*vki){
	vulkaninfo=vki;
	position=glm::vec3(0, 0, 0);
	scale=glm::vec3(1., 1., 1.);
	rotation=glm::quat(1., 0., 0., 0.);
	vertices=std::vector<Vertex>();
	tris=std::vector<Tri>();
	recalculateModelMatrix();
	updateBuffers();
	initCommandBuffers();
	texInit(256, 256, 256);
}

Mesh::Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki, uint32_t dir, uint32_t nir, uint32_t hir){
	vulkaninfo=vki;
	position=p;
	scale=glm::vec3(1., 1., 1.);
	rotation=glm::quat(1., 0., 0., 0.);
	loadOBJ(filepath);
	recalculateModelMatrix();
	updateBuffers();
	initCommandBuffers();
	GraphicsHandler::VKSubInitUniformBuffer(
			&descriptorsets,
			0,
			sizeof(MeshUniformBuffer),
			1,
			&uniformbuffers,
			&uniformbuffermemories,
			GraphicsHandler::vulkaninfo.primarygraphicspipeline.objectdsl);
	texInit(dir, nir, hir);
}

Mesh::Mesh(glm::vec3 p){
	vulkaninfo=&GraphicsHandler::vulkaninfo;
	position=p;
	scale=glm::vec3(1., 1., 1.);
	rotation=glm::quat(1., 0., 0. , 0.);
	recalculateModelMatrix();
	diffusetexture={};
	normaltexture={};
	initCommandBuffers();
	initDescriptorSets();
	texInit(256, 256, 256);
}

Mesh::~Mesh(){
	//TODO: actually fucking write destructors
}

void Mesh::initCommandBuffers(){
	commandbuffers=new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo cmdbufallocinfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.commandpool,
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		MAX_FRAMES_IN_FLIGHT
	};
	vkAllocateCommandBuffers(GraphicsHandler::vulkaninfo.logicaldevice, &cmdbufallocinfo, commandbuffers);
	shadowcommandbuffers=new VkCommandBuffer*[VULKAN_MAX_LIGHTS];
	for(uint8_t i=0;i<VULKAN_MAX_LIGHTS;i++){
		shadowcommandbuffers[i]=new VkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
		VkCommandBufferAllocateInfo shadowcmdbufallocinfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.commandpool,
			VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			MAX_FRAMES_IN_FLIGHT
		};
		vkAllocateCommandBuffers(GraphicsHandler::vulkaninfo.logicaldevice, &shadowcmdbufallocinfo, &shadowcommandbuffers[i][0]);
	}
}

void Mesh::updateBuffers(){
	VkDeviceSize vertexbuffersize=sizeof(Vertex)*vertices.size();
	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&(vulkaninfo->stagingbuffer),
												     vertexbuffersize,
												     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												     &(vulkaninfo->stagingbuffermemory),
												     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void*datatemp;
	vkMapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, 0, vertexbuffersize, 0, &datatemp);
	memcpy(datatemp, (void*)vertices.data(), vertexbuffersize);
	vkUnmapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory);

	//hey i know we dont use this funcion outside of init, but if we did should we be recreating the vbuf?
	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&vertexbuffer,
												     vertexbuffersize,
												     VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
												     &vertexbuffermemory,
												     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	GraphicsHandler::VKHelperCpyBufferToBuffer(&(vulkaninfo->stagingbuffer),
											   &vertexbuffer,
											   vertexbuffersize);

	vkFreeMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffer, nullptr);
	VkDeviceSize indexbuffersize=sizeof(uint16_t)*3*tris.size();
	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&(vulkaninfo->stagingbuffer),
												     indexbuffersize,
												     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												     &(vulkaninfo->stagingbuffermemory),
												     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void*indexdatatemp;
	vkMapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, 0, indexbuffersize, 0, &indexdatatemp);
	uint16_t indextemp[3*tris.size()];
	for(int x=0;x<tris.size();x++){
		for(int y=0;y<3;y++){
			indextemp[3*x+y]=tris[x].vertexindices[y];
		}
	}
	memcpy(indexdatatemp, (void*)indextemp, indexbuffersize);
	vkUnmapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory);

	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&indexbuffer,
												     indexbuffersize,
												     VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
												     &indexbuffermemory,
												     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	GraphicsHandler::VKHelperCpyBufferToBuffer(&(vulkaninfo->stagingbuffer),
											   &indexbuffer,
											   indexbuffersize);

	vkFreeMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffer, nullptr);
}

void Mesh::texInit(uint32_t dir, uint32_t nir, uint32_t hir){
	GraphicsHandler::VKHelperInitTexture(
			&diffusetexture,
			dir, 0,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_DIFFUSE,
			VK_IMAGE_VIEW_TYPE_2D,
			GraphicsHandler::genericsampler);
	GraphicsHandler::VKHelperInitTexture(
			&normaltexture,
			nir, 0,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_NORMAL,
			VK_IMAGE_VIEW_TYPE_2D,
			GraphicsHandler::linearminmagsampler);
	GraphicsHandler::VKHelperInitTexture(
			&heighttexture,
			hir, 0,
			VK_FORMAT_R32_SFLOAT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			TEXTURE_TYPE_HEIGHT,
			VK_IMAGE_VIEW_TYPE_2D,
			GraphicsHandler::linearminmagsampler);
}

void triangulatePolygon(std::vector<glm::vec2>v, std::vector<glm::vec2>&dst);

void Mesh::initDescriptorSets(){
	VkDescriptorSetLayout layoutstemp[vulkaninfo->numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo->numswapchainimages;x++) layoutstemp[x]=vulkaninfo->primarygraphicspipeline.objectdsl;
	descriptorsets=new VkDescriptorSet[vulkaninfo->numswapchainimages];
	VkDescriptorSetAllocateInfo descriptorsetallocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		vulkaninfo->descriptorpool,
		vulkaninfo->numswapchainimages,
		&layoutstemp[0]
	};
	vkAllocateDescriptorSets(vulkaninfo->logicaldevice, &descriptorsetallocinfo, &descriptorsets[0]);
}

void Mesh::rewriteTextureDescriptorSets(){
	VkDescriptorImageInfo imginfo=diffusetexture.getDescriptorImageInfo();
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++){
		VkWriteDescriptorSet write{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				descriptorsets[x],
				1,
				0,
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imginfo,
				nullptr,
				nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
		                       1,
		                       &write,
		                       0,
		                       nullptr);
	}
	imginfo=normaltexture.getDescriptorImageInfo();
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++){
		VkWriteDescriptorSet write{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				descriptorsets[x],
				2,
				0,
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imginfo,
				nullptr,
				nullptr
		};
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
		                       1,
		                       &write,
		                       0,
		                       nullptr);
	}
	imginfo=heighttexture.getDescriptorImageInfo();
	for(uint32_t x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++){
		VkWriteDescriptorSet write{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorsets[x],
			3,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imginfo,
			nullptr,
			nullptr
		};
		vkUpdateDescriptorSets(
				GraphicsHandler::vulkaninfo.logicaldevice,
				1,
				&write,
				0, nullptr);
	}
}

void Mesh::recordDraw(uint8_t fifindex, uint8_t sciindex, VkDescriptorSet*sceneds)const{
	VkCommandBufferInheritanceInfo cmdbufinherinfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.primaryrenderpass,
		0,
		GraphicsHandler::vulkaninfo.primaryframebuffers[sciindex],
		VK_FALSE,
		0,
		0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		&cmdbufinherinfo
	};
	vkBeginCommandBuffer(commandbuffers[fifindex], &cmdbufbegininfo);
	vkCmdBindPipeline(      //would like to only do this bind once, but unlikely to work out...
			commandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipeline);
	vkCmdPushConstants(
			commandbuffers[fifindex],
			GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT|VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PrimaryGraphicsPushConstants),
			&GraphicsHandler::vulkaninfo.primarygraphicspushconstants);
	vkCmdBindDescriptorSets(
			commandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
			0,
			1,
			&sceneds[sciindex],
			0, nullptr);
	//same down to here
	vkCmdBindDescriptorSets(
			commandbuffers[fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.primarygraphicspipeline.pipelinelayout,
			1,
			1,
			&descriptorsets[sciindex],
			0, nullptr);
	VkDeviceSize offsettemp=0u;
	vkCmdBindVertexBuffers(
			commandbuffers[fifindex],
			0,
			1,
			&vertexbuffer,
			&offsettemp);
	vkCmdBindIndexBuffer(
			commandbuffers[fifindex],
			indexbuffer,
			0,
			VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(
			commandbuffers[fifindex],
			tris.size()*3,
			1,
			0,
			0,
			0);
	vkEndCommandBuffer(commandbuffers[fifindex]);
}

void Mesh::recordShadowDraw(uint8_t fifindex, uint8_t sciindex, VkRenderPass renderpass, VkFramebuffer framebuffer, uint8_t lightidx, ShadowmapPushConstants*pc)const{
	VkCommandBufferInheritanceInfo cmdbufinherinfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			renderpass,
			0,
			framebuffer,
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
			&cmdbufinherinfo
	};
	vkBeginCommandBuffer(shadowcommandbuffers[lightidx][fifindex], &cmdbufbegininfo);
	vkCmdBindPipeline(
			shadowcommandbuffers[lightidx][fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.shadowmapgraphicspipeline.pipeline);
	vkCmdPushConstants(
			shadowcommandbuffers[lightidx][fifindex],
			GraphicsHandler::vulkaninfo.shadowmapgraphicspipeline.pipelinelayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ShadowmapPushConstants),
			pc);
	vkCmdBindDescriptorSets(
			shadowcommandbuffers[lightidx][fifindex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsHandler::vulkaninfo.shadowmapgraphicspipeline.pipelinelayout,
			1,
			1,
			&descriptorsets[sciindex],
			0, nullptr);
	VkDeviceSize offsettemp=0u;
	vkCmdBindVertexBuffers(
			shadowcommandbuffers[lightidx][fifindex],
			0,
			1,
			&vertexbuffer,
			&offsettemp);
	vkCmdBindIndexBuffer(
			shadowcommandbuffers[lightidx][fifindex],
			indexbuffer,
			0,
			VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(
			shadowcommandbuffers[lightidx][fifindex],
			tris.size()*3,
			1,
			0,
			0,
			0);
	vkEndCommandBuffer(shadowcommandbuffers[lightidx][fifindex]);
}

void Mesh::generateSteppeMesh(std::vector<glm::vec3>area, std::vector<std::vector<glm::vec3>>waters, double seed){
	vertices=std::vector<Vertex>();
	tris=std::vector<Tri>();
}

void Mesh::recalculateModelMatrix(){
	modelmatrix=glm::translate(position)*glm::toMat4(rotation)*glm::scale(scale);
	uniformbufferdata.modelmatrix=modelmatrix;
}

void Mesh::setPosition(glm::vec3 p){
	position=p;
	recalculateModelMatrix();
}

void Mesh::setRotation(glm::quat r){
	rotation=r;
	recalculateModelMatrix();
}

void Mesh::setScale(glm::vec3 s){
	scale=s;
	recalculateModelMatrix();
}

MeshUniformBuffer Mesh::getUniformBufferData(){
	uniformbufferdata.diffuseuvmatrix=diffusetexture.uvmatrix;
	uniformbufferdata.normaluvmatrix=normaltexture.uvmatrix;
	uniformbufferdata.heightuvmatrix=heighttexture.uvmatrix;
	return uniformbufferdata;
}

void Mesh::cleanUpVertsAndTris(){
//	std::cout<<"cleaning up vertices of mesh @"<<this
//	<<"\n\tremoving duplicates: "<<std::endl;
	uint16_t dupecounter=0;
	uint32_t precleanvertexcount=vertices.size();
	for(int x=0;x<vertices.size();x++){
		for(int y=0;y<x;y++){
			if(y!=x&&vertices[x]==vertices[y]){
				for(int z=0;z<tris.size();z++){
					for(int w=0;w<3;w++){
						if(tris[z].vertexindices[w]==x) tris[z].vertexindices[w]=y;
						else if(tris[z].vertexindices[w]>x) tris[z].vertexindices[w]--;
					}
				}
				vertices.erase(vertices.begin()+x);
				dupecounter++;
//				std::cout<<"\r\t";
				for(float_t perdec=0;perdec<10;perdec++){
					if((float)x/(float)precleanvertexcount*10>perdec) std::cout<<"\u2588";
					else std::cout<<" ";
				}
//				std::cout<<dupecounter<<" duplicates removed ("<<((float)dupecounter/(float)precleanvertexcount*100.0)<<"% size decrease)\n";
			}
		}
	}
//	std::cout<<std::endl;

	bool adjacencyfound;
	for(int x=0;x<tris.size();x++){
		for(int y=0;y<tris.size();y++){
			if(x!=y){
				adjacencyfound=false;
				for(int z=0;z<3;z++){
					for(int w=0;w<3;w++){
						if(vertices[tris[x].vertexindices[z]].position==vertices[tris[y].vertexindices[w]].position){
							tris[x].adjacencies.push_back(&tris[y]);
//							std::cout<<"adjacency found between tri "<<x<<" and tri "<<y<<std::endl;
							adjacencyfound=true;
							break;
						}
					}
					if(adjacencyfound) break;
				}
			}
		}
		std::cout<<"\rfinding adjacencies (tri "<<(x+1)<<" of "<<tris.size()<<')';
	}
	std::cout<<std::endl;

	for(uint32_t x=0;x<tris.size();x++){
		for(uint16_t y=0;y<3;y++){
			tris[x].vertices[y]=&vertices[tris[x].vertexindices[y]];
			std::cout<<"\rgiving tris vertex addresses (tri "<<(x+1)<<" of "<<tris.size()<<')';
		}
	}
	std::cout<<std::endl;

	for(auto&t:tris){
		t.algebraicnormal=glm::normalize(glm::cross(vertices[t.vertexindices[1]].position-vertices[t.vertexindices[0]].position,
		                                            vertices[t.vertexindices[2]].position-vertices[t.vertexindices[0]].position));
	}
}       //add polished print statements here later to show process/times

void Mesh::loadOBJ(const char*filepath){
	tris=std::vector<Tri>(); vertices=std::vector<Vertex>();
	std::vector<unsigned int> vertexindices, uvindices, normalindices;
	std::vector<glm::vec3> vertextemps, normaltemps;
	std::vector<glm::vec2> uvtemps;
	FILE*obj=fopen(filepath, "r");
	if(obj==nullptr){
		std::cout<<"Loading OBJ Failure ("<<filepath<<")\n";
		return;
	}
	while(true){
		char lineheader[128];//128?
		int res=fscanf(obj, "%s", lineheader);
		if(res==EOF){break;}
		if(strcmp(lineheader, "v")==0){
			glm::vec3 vertex;
			fscanf(obj, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertextemps.push_back(vertex);
		}
		else if(strcmp(lineheader, "vt")==0){
			glm::vec2 uv;
			fscanf(obj, "%f %f\n", &uv.x, &uv.y);
			uvtemps.push_back(uv);
		}
		else if(strcmp(lineheader, "vn")==0){
			glm::vec3 normal;
			fscanf(obj, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			normaltemps.push_back(normal);
		}
		else if(strcmp(lineheader, "f")==0){
			unsigned int vertidx[3], uvidx[3], normidx[3];
			int matches=fscanf(obj, "%d/%d/%d %d/%d/%d %d/%d/%d", &vertidx[0], &uvidx[0], &normidx[0],
					  &vertidx[1], &uvidx[1], &normidx[1],
					  &vertidx[2], &uvidx[2], &normidx[2]);
			if(matches!=9){
				std::cout<<"OBJ Format Failure ("<<filepath<<")\n";
				return;
			}
			for(auto&v:vertidx) vertexindices.push_back(v-1);
			for(auto&n:normidx) normalindices.push_back(n-1);
			for(auto&u:uvidx) uvindices.push_back(u-1);
		}
	}

	for(uint16_t x=0;x<vertexindices.size()/3;x++){
		tris.push_back({});
		for(uint16_t y=0;y<3;y++){
			std::cout<<"\rassembling all vertexindices and triangles ("<<(x*3+y+1)<<'/'<<vertexindices.size()<<')';
			vertices.push_back({vertextemps[vertexindices[3*x+y]], normaltemps[normalindices[3*x+y]], uvtemps[uvindices[3*x+y]]});
			tris[tris.size()-1].vertexindices[y]=3*x+y;
		}
	}
	std::cout<<std::endl;

	//any efficiency possible with breaks and stuff??
	std::cout<<"removing duplicates: "<<ANSI_GREEN_FORE<<std::endl;
	uint16_t dupecounter=0;
	for(int x=0;x<vertices.size();x++){
		for(int y=0;y<x;y++){
			if(y!=x&&vertices[x]==vertices[y]){
				for(int z=0;z<tris.size();z++){
					for(int w=0;w<3;w++){
						if(tris[z].vertexindices[w]==x) tris[z].vertexindices[w]=y;
						else if(tris[z].vertexindices[w]>x) tris[z].vertexindices[w]--;
					}
				}
				if(x<vertices.size()) vertices.erase(vertices.begin()+x);
				dupecounter++;
				std::cout<<'\r';
				for(uint8_t bars=0;bars<50;bars++){
					if(float(x+1)/float(vertices.size())*100.>=float(bars)*2.) std::cout<<"\u2588";
					else std::cout<<' ';
				}
			}
		}
	}
	std::cout<<ANSI_RESET_FORE<<std::endl;
	std::cout<<dupecounter<<" duplicates removed! ("<<(float(dupecounter)/float(vertexindices.size())*100.0f)<<" percent size decrease!!!)"<<std::endl;

	std::cout<<"finding tri adjacencies: "<<ANSI_GREEN_FORE<<std::endl;
	bool adjacencyfound;
	for(int x=0;x<tris.size();x++){
		for(int y=0;y<tris.size();y++){
			if(x!=y){
				adjacencyfound=false;
				for(int z=0;z<3;z++){
					for(int w=0;w<3;w++){
						if(vertices[tris[x].vertexindices[z]].position==vertices[tris[y].vertexindices[w]].position){
							tris[x].adjacencies.push_back(&tris[y]);
							adjacencyfound=true;
							break;
						}
					}
					if(adjacencyfound) break;
				}
			}
		}
		std::cout<<'\r';
		for(uint8_t bars=0;bars<50;bars++){
			if(float(x+1)/float(tris.size())*100.>=float(bars*2.)) std::cout<<"\u2588";
			else std::cout<<' ';
		}
//		std::cout<<"\rfinding adjacencies (tri "<<(x+1)<<" of "<<tris.size()<<')';
	}
	std::cout<<ANSI_RESET_FORE<<std::endl;

	for(uint32_t x=0;x<tris.size();x++){
		for(uint16_t y=0;y<3;y++){
			tris[x].vertices[y]=&vertices[tris[x].vertexindices[y]];
			std::cout<<"\rgiving tris vertex addresses (tri "<<(x+1)<<" of "<<tris.size()<<')';
		}
	}
	std::cout<<std::endl;

	for(auto&t:tris){
		t.algebraicnormal=glm::normalize(glm::cross(vertices[t.vertexindices[1]].position-vertices[t.vertexindices[0]].position,
		                                            vertices[t.vertexindices[2]].position-vertices[t.vertexindices[0]].position));
	}

	for(auto&v:vertices){
		for(uint8_t x=0;x<3;x++){
			if(v.position[x]<min[x]) min[x]=v.position[x];
			if(v.position[x]>max[x]) max[x]=v.position[x];
		}
	}
}