//
// Created by Daniel Paavola on 5/30/21.
//

#include "Mesh.h"

VulkanInfo*Mesh::vulkaninfo{};

Mesh::Mesh(VulkanInfo*vki){
	vulkaninfo=vki;
	position=glm::vec3(0, 0, 0);
	vertices=std::vector<Vertex>();
	tris=std::vector<Tri>();
	recalculateModelMatrix();
	shadowtype=RADIUS;
	updateBuffers();
//	initCommandBuffer();
}

Mesh::Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki){
	vulkaninfo=vki;
	position=p;
	loadOBJ(filepath);
	recalculateModelMatrix();
	shadowtype=RADIUS;
	updateBuffers();
	srand(glfwGetTime());
	glm::vec4 noisetexture[512][512];
	for(uint32_t x=0;x<512;x++){
		for(uint32_t y=0;y<512;y++){
//			noisetexture[x][y]=(float(rand())/float(RAND_MAX))*glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
//			noisetexture[x][y]=glm::vec4((float(rand())/float(RAND_MAX)), (float(rand())/float(RAND_MAX)), (float(rand())/float(RAND_MAX)), (float(rand())/float(RAND_MAX)));
//			std::cout<<noisetexture[x][y].x<<'\n';
//			noisetexture[x][y]=glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			noisetexture[x][y]=glm::vec4(sin(x/10.0f), cos(y/10.0f), sin(x/10.0f+3.14f), 1.0f);
		}
	}
	textures.push_back({});
	addTexture(&textures.back(), 512, 512, (void*)(&noisetexture[0][0]), sizeof(glm::vec4), VK_FORMAT_R32G32B32A32_SFLOAT);
	initDescriptorSets();
//	initCommandBuffer();
}

Mesh::~Mesh(){
	for(auto&t:textures){
		vkDestroyImage(vulkaninfo->logicaldevice, t.image, nullptr);
		vkFreeMemory(vulkaninfo->logicaldevice, t.memory, nullptr);
		vkDestroyImageView(vulkaninfo->logicaldevice, t.imageview, nullptr);
		vkDestroySampler(vulkaninfo->logicaldevice, t.sampler, nullptr);
	}
	vkFreeMemory(vulkaninfo->logicaldevice, indexbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, indexbuffer, nullptr);
	vkFreeMemory(vulkaninfo->logicaldevice, vertexbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vertexbuffer, nullptr);
}

void Mesh::updateBuffers(){
	VkDeviceSize vertexbuffersize=sizeof(Vertex)*vertices.size();
	createAndAllocateBuffer(&(vulkaninfo->stagingbuffer), vertexbuffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &(vulkaninfo->stagingbuffermemory), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void*datatemp;
	vkMapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, 0, vertexbuffersize, 0, &datatemp);
	memcpy(datatemp, (void*)vertices.data(), vertexbuffersize);
	vkUnmapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory);

	createAndAllocateBuffer(&vertexbuffer, vertexbuffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexbuffermemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copyBuffer(&(vulkaninfo->stagingbuffer), &vertexbuffer, vertexbuffersize, nullptr);

	vkFreeMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffer, nullptr);
	VkDeviceSize indexbuffersize=sizeof(uint16_t)*3*tris.size();
	createAndAllocateBuffer(&(vulkaninfo->stagingbuffer), indexbuffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &(vulkaninfo->stagingbuffermemory), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void*indexdatatemp;
	vkMapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, 0, indexbuffersize, 0, &indexdatatemp);
	uint16_t indextemp[3*tris.size()];
	for(int x=0;x<tris.size();x++){
		for(int y=0;y<3;y++){
			indextemp[3*x+y]=tris[x].vertices[y];
		}
	}
	memcpy(indexdatatemp, (void*)indextemp, indexbuffersize);
	vkUnmapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory);

	createAndAllocateBuffer(&indexbuffer, indexbuffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexbuffermemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copyBuffer(&(vulkaninfo->stagingbuffer), &indexbuffer, indexbuffersize, nullptr);

	vkFreeMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffer, nullptr);
}

void Mesh::initCommandBuffer(){
	commandbuffers=new VkCommandBuffer[vulkaninfo->numswapchainimages];
	VkCommandBufferAllocateInfo cmdbuffallocinfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		vulkaninfo->commandpool,
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		vulkaninfo->numswapchainimages
	};
	vkAllocateCommandBuffers(vulkaninfo->logicaldevice, &cmdbuffallocinfo, &commandbuffers[0]);
	for(uint32_t x=0;x<vulkaninfo->numswapchainimages;x++){
		VkCommandBufferBeginInfo cmdbuffbegininfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
				&vulkaninfo->commandbufferinheritanceinfos[x]
		};
//		vkBeginCommandBuffer(commandbuffers[x], &cmdbuffbegininfo);
//		vkCmdBindPipeline(commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo->primarygraphicspipeline.pipeline);
//		vkCmdPushConstants(commandbuffers[x], vulkaninfo.primarygraphicspipeline.pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PrimaryGraphicsPushConstants), (void*)(&primarygraphicspushconstants));
//		vkCmdBindDescriptorSets(commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo->primarygraphicspipeline.pipelinelayout, 0, 1, &vulkaninfo->primarygraphicspipeline.descriptorset[x], 0, nullptr);
//		vkCmdBindDescriptorSets(commandbuffers[x], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkaninfo->primarygraphicspipeline.pipelinelayout, 1, 1, &descriptorsets[0], 0, nullptr);
//		VkDeviceSize offsettemp=0u;
//		vkCmdBindVertexBuffers(commandbuffers[x], 0, 1, &vertexbuffer, &offsettemp);
//		vkCmdBindIndexBuffer(commandbuffers[x], indexbuffer, 0, VK_INDEX_TYPE_UINT16);
//		vkCmdDrawIndexed(commandbuffers[x], tris.size()*3, 1, 0, 0, 0);
//		vkEndCommandBuffer(commandbuffers[x]);
	}
}

void Mesh::recordCommandBuffer()const{
	for(uint32_t x=0;x<vulkaninfo->numswapchainimages;x++){
		VkCommandBufferBeginInfo cmdbuffbegininfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				0,
				nullptr
		};
		vkBeginCommandBuffer(commandbuffers[x], &cmdbuffbegininfo);
		vkCmdBindDescriptorSets(commandbuffers[x],
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        vulkaninfo->primarygraphicspipeline.pipelinelayout,
		                        1,
		                        1,
		                        &descriptorsets[0],
		                        0,
		                        nullptr);
		VkDeviceSize offsettemp=0u;
		vkCmdBindVertexBuffers(commandbuffers[x], 0, 1, &vertexbuffer, &offsettemp);
		vkCmdBindIndexBuffer(commandbuffers[x], indexbuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(commandbuffers[x], tris.size()*3, 1, 0, 0, 0);
		vkEndCommandBuffer(commandbuffers[x]);
	}
}

void Mesh::initDescriptorSets(){
	VkDescriptorSetLayout layoutstemp[vulkaninfo->numswapchainimages];
	for(uint32_t x=0;x<vulkaninfo->numswapchainimages;x++) layoutstemp[x]=vulkaninfo->primarygraphicspipeline.meshdescriptorsetlayout;
	//maybe we can get away with one descriptor set, cause its not updating every frame?
	descriptorsets=new VkDescriptorSet[vulkaninfo->numswapchainimages];
	VkDescriptorSetAllocateInfo descriptorsetallocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		vulkaninfo->descriptorpool,
		vulkaninfo->numswapchainimages,
		&layoutstemp[0]
	};
	std::cout<<"alloc of "<<vulkaninfo->numswapchainimages<<" descriptor sets @"<<descriptorsets<<" result: "<<string_VkResult(vkAllocateDescriptorSets(vulkaninfo->logicaldevice, &descriptorsetallocinfo, &descriptorsets[0]))<<std::endl;

	for(uint32_t x=0;x<vulkaninfo->numswapchainimages;x++){
		VkDescriptorImageInfo imginfo={
			textures[0].sampler,
			textures[0].imageview,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		VkWriteDescriptorSet writedescriptorset{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorsets[x],
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imginfo,
			nullptr,
			nullptr
		};
		vkUpdateDescriptorSets(vulkaninfo->logicaldevice, 1, &writedescriptorset, 0, nullptr);
	}
}

void Mesh::createAndAllocateBuffer(VkBuffer*buffer, VkDeviceSize buffersize, VkBufferUsageFlags bufferusage, VkDeviceMemory*buffermemory, VkMemoryPropertyFlags reqprops){
	VkBufferCreateInfo buffercreateinfo{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			buffersize,
			bufferusage,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr
	};
	//things that could possibly be wrong:
	//buffercreateinfo (and therefore:)
	//  flags << unlikely to be the actual problem
	//  buffersize << also unlikely to be the problem (unless we're allocating to small a memory size????? doubtful...)
	//  bufferusage
	//  sharingmode (and therefore:)
	//      queuefamilyindexcount
	//      queuefamilyindices
	//and buffer address << seems most likely issue, as memory access is difficult lol
	//but i explicitly double-checked and the address seems valid
	//
	//to better troubleshoot, we could look at how we successfully use this function elsewhere
	vkCreateBuffer(vulkaninfo->logicaldevice, &buffercreateinfo, nullptr, buffer);
	VkMemoryRequirements memrequirements{};
	vkGetBufferMemoryRequirements(vulkaninfo->logicaldevice, *buffer, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo->physicaldevice, &physicaldevicememprops);
	uint32_t memindex=-1u;
	for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
		if(memrequirements.memoryTypeBits&(1<<x)
		   &&((physicaldevicememprops.memoryTypes[x].propertyFlags&reqprops)==reqprops)){
			memindex=x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo->logicaldevice, &memallocateinfo, nullptr, buffermemory);
	vkBindBufferMemory(vulkaninfo->logicaldevice, *buffer, *buffermemory, 0);
}

void Mesh::copyBuffer(VkBuffer*src, VkBuffer*dst, VkDeviceSize size, VkCommandBuffer*cmdbuff){
	VkBufferCopy region{
		0,
		0,
		size
	};
	VkCommandBufferBeginInfo cmdbuffbegininfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};
	vkBeginCommandBuffer(vulkaninfo->interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdCopyBuffer(vulkaninfo->interimcommandbuffer, *src, *dst, 1, &region);
	vkEndCommandBuffer(vulkaninfo->interimcommandbuffer);
	VkSubmitInfo subinfo{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		0,
		nullptr,
		nullptr,
		1,
		&(vulkaninfo->interimcommandbuffer),
		0,
		nullptr
	};
	//could be using a different queue for these transfer operations
	vkQueueSubmit(vulkaninfo->graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkaninfo->graphicsqueue);
}

void Mesh::copyBufferToImage(VkBuffer*src, VkImage*dst, uint32_t horires, uint32_t vertres){
	VkBufferImageCopy imgcopy{
		0,
		0,
		0,
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			0,
			1
		}, {
			0,
			0,
			0
		}, {
			horires,
			vertres,
			1
		}
	};
	VkCommandBufferBeginInfo cmdbuffbegininfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};
	vkBeginCommandBuffer(vulkaninfo->interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdCopyBufferToImage(vulkaninfo->interimcommandbuffer, *src, *dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgcopy);
	vkEndCommandBuffer(vulkaninfo->interimcommandbuffer);
	VkSubmitInfo subinfo{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		0,
		nullptr,
		nullptr,
		1,
		&(vulkaninfo->interimcommandbuffer),
		0,
		nullptr
	};
	vkQueueSubmit(vulkaninfo->graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkaninfo->graphicsqueue);
}

void Mesh::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldlayout, VkImageLayout newlayout){
	VkCommandBufferBeginInfo cmdbuffbegininfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr
	};
	VkImageMemoryBarrier imgmembarrier{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			0,
			0,
			oldlayout,
			newlayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					1,
					0,
					1
			}
	};
	VkPipelineStageFlags srcmask, dstmask;
	if(oldlayout==VK_IMAGE_LAYOUT_UNDEFINED&&newlayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
		imgmembarrier.srcAccessMask=0;
		imgmembarrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
		srcmask=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstmask=VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(oldlayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL&&newlayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		imgmembarrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
		imgmembarrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
		srcmask=VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstmask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	vkBeginCommandBuffer(vulkaninfo->interimcommandbuffer, &cmdbuffbegininfo);
	vkCmdPipelineBarrier(vulkaninfo->interimcommandbuffer, srcmask, dstmask, 0, 0, nullptr, 0, nullptr, 1, &imgmembarrier);
	vkEndCommandBuffer(vulkaninfo->interimcommandbuffer);
	VkSubmitInfo subinfo{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&(vulkaninfo->interimcommandbuffer),
			0,
			nullptr
	};
	std::cout<<"img layout transition ("<<string_VkImageLayout(oldlayout)<<" to "<<string_VkImageLayout(newlayout)<<") result: "<<string_VkResult(vkQueueSubmit(vulkaninfo->graphicsqueue, 1, &subinfo, VK_NULL_HANDLE))<<std::endl;
	vkQueueWaitIdle(vulkaninfo->graphicsqueue);
}

void Mesh::recalculateModelMatrix(){
	modelmatrix=glm::translate(position);
}

void Mesh::draw(GLuint shaders, bool sendnormals, bool senduvs)const{
//	recordCommandBuffer();
}

void Mesh::setPosition(glm::vec3 p){
	position=p;
	recalculateModelMatrix();
}

void Mesh::addTexture(TextureInfo*texturedst,
					  uint32_t horires,
					  uint32_t vertres,
					  void*data,
					  uint32_t pixelsize,
					  VkFormat format){
	VkImageCreateInfo imgcreateinfo{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		format,
		{horires, vertres, 1},
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,     //perhaps add more tiling control later (e.g. if square po2 text
		VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(vulkaninfo->logicaldevice, &imgcreateinfo, nullptr, &(texturedst->image));
	VkSubresourceLayout subresourcelayout;
	VkImageSubresource imgsubresource{
		VK_IMAGE_ASPECT_COLOR_BIT,
		0,
		0
	};
	vkGetImageSubresourceLayout(vulkaninfo->logicaldevice, texturedst->image, &imgsubresource, &subresourcelayout);
	VkDeviceSize size=subresourcelayout.size;
	std::cout<<"row pitch: "<<subresourcelayout.rowPitch<<"; derived row size: "<<(horires*pixelsize)<<"; size: "<<subresourcelayout.size<<"; offset: "<<subresourcelayout.offset<<std::endl;

	createAndAllocateBuffer(&(vulkaninfo->stagingbuffer), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &(vulkaninfo->stagingbuffermemory), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	//okay this was the root of the problem: i had to make a temp void* instead of just using the function
	//arguement data i pass in; ig vkMapMemory only detects change, not initial value
	void*datatemp;
	vkMapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, 0, size, 0, &datatemp);
	char*dstscanptr=(char*)datatemp, *srcscanptr=(char*)data;
	for(uint32_t x=0;x<vertres;x++){
		memcpy(dstscanptr, srcscanptr, horires*pixelsize);
		dstscanptr+=subresourcelayout.rowPitch;
		srcscanptr+=horires*pixelsize;
	}
	vkUnmapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory);

	VkMemoryRequirements memrequirements{};
	vkGetImageMemoryRequirements(vulkaninfo->logicaldevice, texturedst->image, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo->physicaldevice, &physicaldevicememprops);
	uint32_t memindex=-1u;
	for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
		if(memrequirements.memoryTypeBits&(1<<x)
		   &&((physicaldevicememprops.memoryTypes[x].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)==VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){
			memindex=x;
			break;
		}
	}
	VkMemoryAllocateInfo memallocateinfo{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memrequirements.size,
			memindex
	};
	vkAllocateMemory(vulkaninfo->logicaldevice, &memallocateinfo, nullptr, &(texturedst->memory));
	vkBindImageMemory(vulkaninfo->logicaldevice, texturedst->image, texturedst->memory, 0);

	transitionImageLayout(texturedst->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(&(vulkaninfo->stagingbuffer), &texturedst->image, horires, vertres);
	vkFreeMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffer, nullptr);
	transitionImageLayout(texturedst->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageViewCreateInfo imgviewcreateinfo{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		texturedst->image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	vkCreateImageView(vulkaninfo->logicaldevice, &imgviewcreateinfo, nullptr, &(texturedst->imageview));

	VkSamplerCreateInfo samplercreateinfo{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		nullptr,
		0,
		VK_FILTER_NEAREST,
		VK_FILTER_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		0.0f,
		VK_FALSE,
		1.0f,
		VK_FALSE,
		VK_COMPARE_OP_ALWAYS,
		0.0f,
		1.0f,
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_FALSE
	};
	vkCreateSampler(vulkaninfo->logicaldevice, &samplercreateinfo, nullptr, &(texturedst->sampler));
}

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
		tris.push_back({{UINT16_MAX, UINT16_MAX, UINT16_MAX}, std::vector<Tri*>(), glm::vec3(0, 0, 0)});
		for(uint16_t y=0;y<3;y++){
			std::cout<<"\rassembling all vertices and triangles ("<<(x*3+y+1)<<'/'<<vertexindices.size()<<')';
			vertices.push_back({vertextemps[vertexindices[3*x+y]], normaltemps[normalindices[3*x+y]], uvtemps[uvindices[3*x+y]]});
			tris[tris.size()-1].vertices[y]=3*x+y;
		}
	}
	std::cout<<std::endl;

	//any efficiency possible with breaks and stuff??
	uint16_t dupecounter=0;
	for(int x=0;x<vertices.size();x++){
		for(int y=0;y<x;y++){
			if(y!=x&&vertices[x]==vertices[y]){
				for(int z=0;z<tris.size();z++){
					for(int w=0;w<3;w++){
						if(tris[z].vertices[w]==x) tris[z].vertices[w]=y;
						else if(tris[z].vertices[w]>x) tris[z].vertices[w]--;
					}
				}
				vertices.erase(vertices.begin()+x);
				dupecounter++;
				std::cout<<'\r'<<dupecounter<<" duplicates removed! ("<<(float(dupecounter)/float(vertexindices.size())*100.0f)<<" percent size decrease!!!)";
			}
		}
	}
	std::cout<<std::endl;

	bool adjacencyfound;
	for(int x=0;x<tris.size();x++){
		for(int y=0;y<tris.size();y++){
			if(x!=y){
				adjacencyfound=false;
				for(int z=0;z<3;z++){
					for(int w=0;w<3;w++){
						if(vertices[tris[x].vertices[z]].position==vertices[tris[y].vertices[w]].position){
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

	for(auto&t:tris){
		t.algebraicnormal=glm::normalize(glm::cross(vertices[t.vertices[1]].position-vertices[t.vertices[0]].position,
							   vertices[t.vertices[2]].position-vertices[t.vertices[0]].position));
//		std::cout<<"algebraic normal: <"<<t.algebraicnormal.x<<", "<<t.algebraicnormal.y<<", "<<t.algebraicnormal.z<<">\n";
	}
}