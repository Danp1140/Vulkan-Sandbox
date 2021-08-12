//
// Created by Daniel Paavola on 8/5/21.
//

#include "Ocean.h"

Ocean::Ocean(glm::vec3 pos, glm::vec2 b, Mesh*s): Mesh(pos){
	bounds=b;
	shore=s;
	float subdivwidth=1.0/(float)OCEAN_PRE_TESS_SUBDIV;
	glm::vec2 dbounds=bounds/(float)OCEAN_PRE_TESS_SUBDIV;
	for(uint16_t y=0;y<OCEAN_PRE_TESS_SUBDIV+1;y++){
		for(uint16_t x=0;x<OCEAN_PRE_TESS_SUBDIV+1;x++){
			vertices.push_back({pos+glm::vec3(dbounds.x*x, 0.0, dbounds.y*y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(x*subdivwidth, y*subdivwidth)});
		}
	}
	for(uint16_t y=0;y<OCEAN_PRE_TESS_SUBDIV;y++){
		for(uint16_t x=0;x<OCEAN_PRE_TESS_SUBDIV;x++){
			tris.push_back({});
			tris.back().vertexindices[0]=(OCEAN_PRE_TESS_SUBDIV+1)*y+x;
			tris.back().vertexindices[1]=(OCEAN_PRE_TESS_SUBDIV+1)*(y+1)+x;
			tris.back().vertexindices[2]=(OCEAN_PRE_TESS_SUBDIV+1)*(y+1)+x+1;
			tris.push_back({});
			tris.back().vertexindices[0]=(OCEAN_PRE_TESS_SUBDIV+1)*(y+1)+x+1;
			tris.back().vertexindices[1]=(OCEAN_PRE_TESS_SUBDIV+1)*y+x+1;
			tris.back().vertexindices[2]=(OCEAN_PRE_TESS_SUBDIV+1)*y+x;
		}
	}
	cleanUpVertsAndTris();
//	vertices.push_back({pos, glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 0.0)});
//	vertices.push_back({pos+glm::vec3(0.0, 0.0, bounds.y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 1.0)});
//	vertices.push_back({pos+glm::vec3(bounds.x, 0.0, bounds.y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 1.0)});
//	vertices.push_back({pos+glm::vec3(bounds.x, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 0.0)});
//	tris.push_back({{0, 1, 2},
//	                {&vertices[0], &vertices[1], &vertices[2]},
//	                std::vector<Tri*>(),
//                    glm::cross(vertices[1].position-vertices[0].position, vertices[2].position-vertices[0].position)});
//	tris.push_back({{2, 3, 0},
//	                {&vertices[2], &vertices[3], &vertices[0]},
//	                std::vector<Tri*>(),
//	                glm::cross(vertices[3].position-vertices[2].position, vertices[0].position-vertices[2].position)});
//	tris[0].adjacencies.push_back(&tris[1]);
//	tris[1].adjacencies.push_back(&tris[0]);
	updateBuffers();
	initDescriptorSets();
}

void Ocean::initDescriptorSets(){
	VkDescriptorSetLayout dsltemp[GraphicsHandler::vulkaninfo.numswapchainimages];
	for(unsigned char x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++) dsltemp[x]=GraphicsHandler::vulkaninfo.oceangraphicspipeline.objectdsl;
	VkDescriptorSetAllocateInfo descriptorsetallocinfo{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.descriptorpool,
		GraphicsHandler::vulkaninfo.numswapchainimages,
		&dsltemp[0]
	};
	vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, &descriptorsetallocinfo, &descriptorsets[0]);
}

void Ocean::rewriteAllDescriptorSets(){
	for(unsigned char x=0;x<GraphicsHandler::vulkaninfo.numswapchainimages;x++){
		VkDescriptorImageInfo imginfo=heightmap.getDescriptorImageInfo();
		VkWriteDescriptorSet writedescset{
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
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);

		imginfo=normalmap.getDescriptorImageInfo();
		writedescset.dstBinding=1;
		writedescset.pImageInfo=&imginfo;
		vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &writedescset, 0, nullptr);
	}
}

void Ocean::rewriteDescriptorSet(uint32_t index){
	VkDescriptorImageInfo imginfo=heightmap.getDescriptorImageInfo();
	VkWriteDescriptorSet write{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		descriptorsets[index],
		0,
		0,
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		&imginfo,
		nullptr,
		nullptr,
	};
	vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &write, 0, nullptr);

	imginfo=normalmap.getDescriptorImageInfo();
	write.dstBinding=1;
	write.pImageInfo=&imginfo;
	//could p write both at once
	vkUpdateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice, 1, &write, 0, nullptr);
}