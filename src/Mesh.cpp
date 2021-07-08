//
// Created by Daniel Paavola on 5/30/21.
//

#include "Mesh.h"

VulkanInfo*Mesh::vulkaninfo{};

Mesh::Mesh(VulkanInfo*vki){
	vulkaninfo=vki;
	position=glm::vec3(0, 0, 0);
	vertices=std::vector<Vertex>();
	vertices.push_back({glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec2(0, 0)});
	vertices.push_back({glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec2(0, 0)});
	vertices.push_back({glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec2(0, 0)});
	vertices.push_back({glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)});
	tris=std::vector<Tri>();
	tris.push_back({{0, 1, 2}});
	tris.push_back({{0, 1, 3}});
	tris.push_back({{0, 2, 3}});
	recalculateModelMatrix();
	shadowtype=RADIUS;
	updateBuffers();
	initCommandBuffer();
}

Mesh::Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki){
	vulkaninfo=vki;
	position=p;
	loadOBJ(filepath);
	recalculateModelMatrix();
	shadowtype=RADIUS;
	updateBuffers();
	initCommandBuffer();
	srand(glfwGetTime());
	glm::vec4 noisetexture[512][512];
	for(uint32_t x=0;x<512;x++){
		for(uint32_t y=0;y<512;y++){
			noisetexture[x][y]=rand()*glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	addTexture(512, (void*)(&noisetexture[0][0]));
}

Mesh::~Mesh(){
	vkFreeCommandBuffers(vulkaninfo->logicaldevice, vulkaninfo->commandpool, 1, &commandbuffer);
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
	createAndAllocateBuffer(&indexbuffer, indexbuffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &indexbuffermemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copyBuffer(&(vulkaninfo->stagingbuffer), &indexbuffer, indexbuffersize, nullptr);
	vkFreeMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, nullptr);
	vkDestroyBuffer(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffer, nullptr);
}

void Mesh::initCommandBuffer(){
	VkCommandBufferAllocateInfo commandbufferallocateinfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		vulkaninfo->commandpool,
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		1
	};
	vkAllocateCommandBuffers(vulkaninfo->logicaldevice, &commandbufferallocateinfo, &commandbuffer);
	VkCommandBufferBeginInfo commandbufferbegininfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		0,
		nullptr
	};
	vkBeginCommandBuffer(commandbuffer, &commandbufferbegininfo);
	VkDeviceSize offsettemp=0u;
	vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vertexbuffer, &offsettemp);
	vkCmdBindIndexBuffer(commandbuffer, indexbuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(commandbuffer, tris.size()*3, 1, 0, 0, 0);
	vkEndCommandBuffer(commandbuffer);
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
	vkCreateBuffer(vulkaninfo->logicaldevice, &buffercreateinfo, nullptr, buffer);
	VkMemoryRequirements memrequirements{};
	vkGetBufferMemoryRequirements(vulkaninfo->logicaldevice, *buffer, &memrequirements);
	VkPhysicalDeviceMemoryProperties physicaldevicememprops{};
	vkGetPhysicalDeviceMemoryProperties(vulkaninfo->physicaldevice, &physicaldevicememprops);
	//would like to understand below checks better...
	uint32_t memindex=-1u;
	for(uint32_t x=0;x<physicaldevicememprops.memoryTypeCount;x++){
		if(memrequirements.memoryTypeBits&(1<<x)
		   &&((physicaldevicememprops.memoryTypes[x].propertyFlags&reqprops)==reqprops)){
			memindex=x;
//			std::cout<<"mem index: "<<x<<std::endl;
			break;
		}
	}
	//would like to understand memory dynamics better...
	//what is best optimization for static/const (or dynamic) meshes?
	//when can memory be accessed/changed?
	//when can memory be unmapped and what are the benefits?
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

void Mesh::copyBufferToImage(VkBuffer*src, VkImage*dst, uint32_t resolution){
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
			resolution,
			resolution,
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
	vkQueueSubmit(vulkaninfo->graphicsqueue, 1, &subinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkaninfo->graphicsqueue);
}

void Mesh::recalculateModelMatrix(){
	modelmatrix=glm::translate(position);
}

void Mesh::draw(GLuint shaders, bool sendnormals, bool senduvs)const{
	//add LOD param later??
	//this should be as efficient as possible, cause its getting called a shit-ton
}

void Mesh::setPosition(glm::vec3 p){
	position=p;
	recalculateModelMatrix();
}

void Mesh::addTexture(uint32_t resolution, void*data){
	//would like to understand */all/* of this better...
	VkBuffer stagingbuffer;
	VkDeviceMemory stagingbuffermemory;
	VkDeviceSize size=resolution*resolution*4;
	createAndAllocateBuffer(&stagingbuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingbuffermemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	//if this doesnt work, make a temp void*, memcpy after the map, then unmap
	vkMapMemory(vulkaninfo->logicaldevice, stagingbuffermemory, 0, size, 0, &data);
	vkUnmapMemory(vulkaninfo->logicaldevice, stagingbuffermemory);
	TextureInfo texinfotemp={VK_NULL_HANDLE, VK_NULL_HANDLE};
	VkImageCreateInfo imgcreateinfo{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_R8G8B8A8_SRGB,
		{resolution, resolution, 1},
		1,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(vulkaninfo->logicaldevice, &imgcreateinfo, nullptr, &(texinfotemp.image));
	VkMemoryRequirements memrequirements{};
	vkGetImageMemoryRequirements(vulkaninfo->logicaldevice, texinfotemp.image, &memrequirements);
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
	vkAllocateMemory(vulkaninfo->logicaldevice, &memallocateinfo, nullptr, &(texinfotemp.memory));
	vkBindImageMemory(vulkaninfo->logicaldevice, texinfotemp.image, texinfotemp.memory, 0);


	transitionImageLayout(texinfotemp.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(&(vulkaninfo->stagingbuffer), &texinfotemp.image, resolution);
	transitionImageLayout(texinfotemp.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageViewCreateInfo imgviewcreateinfo{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		texinfotemp.image,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R8G8B8A8_SRGB,
		{},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};
	vkCreateImageView(vulkaninfo->logicaldevice, &imgviewcreateinfo, nullptr, &(texinfotemp.imageview));

	VkSamplerCreateInfo samplercreateinfo{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		nullptr,
		0,
		VK_FILTER_NEAREST,
		VK_FILTER_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0f,
		VK_TRUE,
		4.0f,
		VK_FALSE,
		VK_COMPARE_OP_ALWAYS,
		0.0f,
		0.0f,
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_FALSE
	};
	vkCreateSampler(vulkaninfo->logicaldevice, &samplercreateinfo, nullptr, &(texinfotemp.sampler));

	//hopefully passing by value is okay here
	textures.push_back(texinfotemp);
}

void Mesh::loadOBJ(const char*filepath){
	/* writing this function has introduced some problematic parts of how we go about sending/rendering info w/ the gpu.
	 * worth considering options here. would like to be able to do seperate indexing for vertex position, uv, and normal,
	 * but not sure if that's possible with opengl....
	*/
	tris=std::vector<Tri>(); vertices=std::vector<Vertex>();
	std::vector<unsigned int> vertexindices, uvindices, normalindices;
	std::vector<glm::ivec3> combinations;
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
//			std::cout<<vertextemps.size()<<" vertices, "<<uvtemps.size()<<" uvs, "<<normaltemps.size()<<" normals"<<std::endl;
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
	for(int x=0;x<vertexindices.size();x+=3){
		uint16_t duplicate[3]={(uint16_t)-1u, (uint16_t)-1u, (uint16_t)-1u};
		for(int y=0;y<3;y++){
			for(int z=0;z<combinations.size();z++){
				if(combinations[z]==glm::ivec3(vertexindices[x+y], uvindices[x+y], normalindices[x+y])){
					duplicate[y]=z;
				}
			}
		}
		uint16_t triindices[3]={(uint16_t)-1u, (uint16_t)-1u, (uint16_t)-1u};
		for(int y=0;y<3;y++){
//			std::cout<<"loading... ("<<combinations.size()<<" combinations to check thru...)"<<std::endl;
			if(duplicate[y]==(uint16_t)-1u){
				vertices.push_back({vertextemps[vertexindices[x+y]], normaltemps[normalindices[x+y]], uvtemps[uvindices[x+y]]});
				triindices[y]=vertices.size()-1;
				combinations.push_back(glm::ivec3(vertexindices[x+y], normalindices[x+y], uvindices[x+y]));
			}
			else triindices[y]=duplicate[y];
		}
		tris.push_back({triindices[0], triindices[1], triindices[2]});
	}
//	for(auto&t:tris){
//		std::cout<<vertices[t.vertices[0]].getString()<<"<=>"<<vertices[t.vertices[1]].getString()<<"<=>"<<vertices[t.vertices[2]].getString()<<std::endl;
//	}
}

void Mesh::loadOBJLoadtimeOptimized(const char*filepath){
	tris=std::vector<Tri>(); vertices=std::vector<Vertex>();
	std::vector<unsigned int> vertexindices, uvindices, normalindices;
	std::vector<glm::ivec3> combinations;
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
//			std::cout<<vertextemps.size()<<" vertices, "<<uvtemps.size()<<" uvs, "<<normaltemps.size()<<" normals"<<std::endl;
			unsigned int vertidx[3], uvidx[3], normidx[3];
			int matches=fscanf(obj, "%d/%d/%d %d/%d/%d %d/%d/%d", &vertidx[0], &uvidx[0], &normidx[0],
			                   &vertidx[1], &uvidx[1], &normidx[1],
			                   &vertidx[2], &uvidx[2], &normidx[2]);
			if(matches!=9){
				std::cout<<"OBJ Format Failure ("<<filepath<<")\n";
				return;
			}
			for(int x=0;x<3;x++){
				vertices.push_back({glm::vec3(vertextemps[vertidx[x]]), glm::vec3(normaltemps[normidx[x]]), glm::vec2(uvtemps[uvidx[x]])});
			}
			tris.push_back({{(uint16_t)(vertices.size()-3), (uint16_t)(vertices.size()-2), (uint16_t)(vertices.size()-1)}});
		}
	}
}