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
}

Mesh::Mesh(const char*filepath, glm::vec3 p, VulkanInfo*vki){
	vulkaninfo=vki;
	position=p;
	loadOBJ(filepath);
	recalculateModelMatrix();
	shadowtype=RADIUS;
	updateBuffers();
	srand(glfwGetTime());
	glm::vec4*noisetexture=(glm::vec4*)malloc(2048*2048*sizeof(glm::vec4));
	for(uint32_t x=0;x<2048;x++){
		for(uint32_t y=0;y<2048;y++){
//			noisetexture[x][y]=(float(rand())/float(RAND_MAX))*glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			noisetexture[x*2048+y]=glm::vec4((float(rand())/float(RAND_MAX)), (float(rand())/float(RAND_MAX)), (float(rand())/float(RAND_MAX)), 1.0f);
//			noisetexture[x][y]=glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
//			noisetexture[x*512+y]=glm::vec4(sin(x/10.0f), cos(y/10.0f), sin(x/10.0f+3.14f), 1.0f);
//			noisetexture[x*512+y]=glm::vec4()
		}
	}
	textures.push_back({});
	//format and element size are kinda redundant
	GraphicsHandler::VKHelperInitTexture(&textures.back(), 2048, 2048, (void*)(&noisetexture[0]), sizeof(glm::vec4), VK_FORMAT_R32G32B32A32_SFLOAT);
	free(noisetexture);
	initDescriptorSets();
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
	GraphicsHandler::VKHelperCreateAndAllocateBuffer(&(vulkaninfo->stagingbuffer),
												     vertexbuffersize,
												     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												     &(vulkaninfo->stagingbuffermemory),
												     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void*datatemp;
	vkMapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory, 0, vertexbuffersize, 0, &datatemp);
	memcpy(datatemp, (void*)vertices.data(), vertexbuffersize);
	vkUnmapMemory(vulkaninfo->logicaldevice, vulkaninfo->stagingbuffermemory);

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
			indextemp[3*x+y]=tris[x].vertices[y];
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

void Mesh::recalculateModelMatrix(){
	modelmatrix=glm::translate(position);
}

void Mesh::setPosition(glm::vec3 p){
	position=p;
	recalculateModelMatrix();
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