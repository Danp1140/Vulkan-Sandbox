//
// Created by Daniel Paavola on 12/25/22.
//

#include "Terrain.h"

Terrain::Terrain () : 
	Mesh(),
	treestoragebuffer({}),
	voxelstoragebuffer({}),
	meminfostoragebuffer({}),
	genpushconstants({}),
	computeds(VK_NULL_HANDLE),
	memoryavailable(false),
	compfif(0xff) {
	numnodes = 4096u; // not sure how we reconcile this with the NODE_HEAP_SIZE macro in the .h
	numleaves = 1u;
	numvoxels = 3u;

	memoryavailable = false;
	// unsure of default compfif value

	genpushconstants.heapsize = numnodes;

	// unsure if this is the right objdsl to be handing over to Mesh for default init
	// in fact im pretty sure its not
	// TODO: change this objdsl to that of the generated terrain later
	initDescriptorSets(VK_NULL_HANDLE);

	treestoragebuffer.elemsize = NODE_SIZE;
	treestoragebuffer.numelems = numnodes;
	GraphicsHandler::VKSubInitStorageBuffer(treestoragebuffer);
	voxelstoragebuffer.elemsize = VOXEL_SIZE;
	voxelstoragebuffer.numelems = numvoxels;
	GraphicsHandler::VKSubInitStorageBuffer(voxelstoragebuffer);
	meminfostoragebuffer.elemsize = sizeof(uint32_t);
	meminfostoragebuffer.numelems = 3 + ALLOC_INFO_BUF_SIZE;
	GraphicsHandler::VKSubInitStorageBuffer(meminfostoragebuffer);

	// note: in previous code, we updated the tree SB twice, and never the voxel SB...
	// i think i was just being stupid but making a note in case very odd problems arise lol
	GraphicsHandler::updateDescriptorSet(
		computeds,
		{0, 1, 2},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
		{{}, {}, {}},
		{treestoragebuffer.getDescriptorBufferInfo(),
		voxelstoragebuffer.getDescriptorBufferInfo(),
		meminfostoragebuffer.getDescriptorBufferInfo()});

	setupHeap();
}

void Terrain::setupHeap() {
	/* doubly-linked list heap setup
	 * root's parent stores ptr to first free
	 * free blocks are 8 nodes long, with only the first node containing information as follows
	 * 	- voxel = -1u if free
	 * 	- parent stores ptr to next free
	 * 	- childidx stores ptr to prev free
	 * 	- above two are -1u if no such block exists
	 */
	void* heap;
	vkMapMemory(
			GraphicsHandler::vulkaninfo.logicaldevice,
			treestoragebuffer.memory,
			0u,
			numnodes * NODE_SIZE,
			0,
			&heap);
	Node* nodeheap = reinterpret_cast<Node*>(heap);
	uint32_t i;
	for (i = 1; i < NODE_HEAP_SIZE; i += 8) {
		nodeheap[i].voxel = -1u;
		nodeheap[i].parent = i + 8;
		nodeheap[i].childidx = i - 8;
	}
	nodeheap[8].childidx = -1u;
	nodeheap[i - 8].parent = -1u;
	nodeheap[0].parent = 1u;
	nodeheap[0].childidx = 1u;
	subdivide(0u, nodeheap);
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, treestoragebuffer.memory);
	void *tmp;
	vkMapMemory(GraphicsHandler::vulkaninfo.logicaldevice,
		meminfostoragebuffer.memory,
		0u,
		sizeof(uint32_t) * (3 + ALLOC_INFO_BUF_SIZE),
		0,
		&tmp);
	uint32_t* meminfo = reinterpret_cast<uint32_t*>(tmp);
	*meminfo = 0u; *(meminfo + 1) = 0u; *(meminfo + 2) = 0u;
	*(meminfo + 3) = -1u;
	*(meminfo + 3 + ALLOC_INFO_BUF_SIZE / 2) = -1u;
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, meminfostoragebuffer.memory);
}

void Terrain::createComputePipeline () {
	VkDescriptorSetLayoutBinding objdslbindings[3] {{
		0,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}, {
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}, {
		2,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr
	}, {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		3,
		&objdslbindings[0]
	}};
	VkSpecializationMapEntry specmap [2] {{
		0,
		0,
		sizeof(uint32_t)
	}, {
		1,
		sizeof(uint32_t),
		sizeof(uint32_t)
	}};
	uint32_t specdata[2] = {NODE_HEAP_SIZE, ALLOC_INFO_BUF_SIZE};

	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	pii.shaderfilepathprefix = "tgvoxel";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TerrainGenPushConstants)};
	pii.specinfo = {
		2, &specmap[0],
		2 * sizeof(uint32_t), reinterpret_cast<void*>(&specdata[0])
	};

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.terraingencomputepipeline, pii);
}

void Terrain::createGraphicsPipeline () {
	VkDescriptorSetLayoutBinding objdslbindings[3] {{
		0,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}, {
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}, {
		2,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	VkDescriptorSetLayoutCreateInfo dslcreateinfos[2] {{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr
	}, {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		3,
		&objdslbindings[0]
	}};

	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pii.shaderfilepathprefix = "voxeltroubleshooting";
	pii.descsetlayoutcreateinfos = &dslcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)};

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.voxeltroubleshootingpipeline, pii);
}

void Terrain::recordCompute (cbRecData data, VkCommandBuffer& cb) {
	VkCommandBufferInheritanceInfo cmdbufinherinfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			VK_NULL_HANDLE,
			-1u,
			VK_NULL_HANDLE,
			VK_FALSE,
			0,
			0
	};
	VkCommandBufferBeginInfo cmdbufbegininfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			0,
			&cmdbufinherinfo
	};
	vkBeginCommandBuffer(cb, &cmdbufbegininfo);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, data.pipeline.pipeline);
	vkCmdBindDescriptorSets(cb,
							VK_PIPELINE_BIND_POINT_COMPUTE,
							data.pipeline.pipelinelayout,
							0,
							1,
							&data.descriptorset,
							0,
							nullptr);
	vkCmdPushConstants(
			cb,
			data.pipeline.pipelinelayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(TerrainGenPushConstants),
			data.pushconstantdata);
	TerrainGenPushConstants* pctemp = reinterpret_cast<TerrainGenPushConstants*>(data.pushconstantdata);
	if (pctemp->phase == 0) vkCmdDispatch(cb, pctemp->numleaves, 1, 1);
	else if (pctemp->phase == 1) vkCmdDispatch(cb, pctemp->numleaves / 8, 1, 1);	
	// std::cout << "recording TG compute with " << pctemp->numleaves << " invocs" << std::endl;
	vkEndCommandBuffer(cb);
}

void Terrain::recordTroubleshootDraw (cbRecData data, VkCommandBuffer& cb) {
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
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipeline.pipeline);
	vkCmdBindDescriptorSets(cb,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		data.pipeline.pipelinelayout,
		0,
		1,
		&data.descriptorset,
		0,
		nullptr);
	vkCmdPushConstants(cb,
					   data.pipeline.pipelinelayout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(glm::mat4),
					   data.pushconstantdata);
	vkCmdDraw(cb, data.numtris * 3u, 1, 0, 0);
	vkEndCommandBuffer(cb);
}

void Terrain::initDescriptorSets (VkDescriptorSetLayout objdsl) {
	if (computeds != VK_NULL_HANDLE)
		vkFreeDescriptorSets(
			GraphicsHandler::vulkaninfo.logicaldevice,
			GraphicsHandler::vulkaninfo.descriptorpool,
			1, &computeds);
	VkDescriptorSetAllocateInfo descriptorsetallocinfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		GraphicsHandler::vulkaninfo.descriptorpool,
		1, &GraphicsHandler::vulkaninfo.terraingencomputepipeline.objectdsl
	};
	vkAllocateDescriptorSets(
		GraphicsHandler::vulkaninfo.logicaldevice,
		&descriptorsetallocinfo,
		&computeds);

}

// TODO: delete this function
void* Terrain::getNodeHeapPtr () {
	void* ptr;
	vkMapMemory(
			GraphicsHandler::vulkaninfo.logicaldevice,
			treestoragebuffer.memory,
			0u,
			numnodes * NODE_SIZE,
			0,
			&ptr);
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, treestoragebuffer.memory);
	return ptr;
}

void Terrain::updateNumLeaves () {
	Node* root = reinterpret_cast<Node*>(getNodeHeapPtr());
	if (root->childidx == 0) return;
	numleaves = root->childidx;
	genpushconstants.numleaves = numleaves;
}

glm::vec3 childDirection (uint8_t childidx) {
	if (childidx == 0) return glm::vec3(1., 1., 1.);
	if (childidx == 1) return glm::vec3(-1., 1., 1.);
	if (childidx == 2) return glm::vec3(-1., -1., 1.);
	if (childidx == 3) return glm::vec3(1., -1., 1.);
	if (childidx == 4) return glm::vec3(1., 1., -1.);
	if (childidx == 5) return glm::vec3(-1., 1., -1.);
	if (childidx == 6) return glm::vec3(-1., -1., -1.);
	return glm::vec3(1., -1., -1.);
}

glm::vec3 Terrain::getLeafPos (uint32_t leafidx, uint32_t& nodeidx, Node* heap) {
	uint32_t tempptr = 0u, depth = 0u;
	glm::vec3 position = glm::vec3(0.);
	// go to bottom leftmost
	while (heap[tempptr].children[0] != -1u) {
		tempptr = heap[tempptr].children[0];
		depth++;
		position += glm::vec3(1.) * pow(0.5, depth);
	}
	uint8_t childidx = 0u;
	for (uint32_t i = 0u; i < leafidx; i++) {
		// if we can continue on the same level, do so
		if (childidx != 7) {
			position -= childDirection(childidx) * pow(0.5, depth);
			childidx++;
			position += childDirection(childidx) * pow(0.5, depth);
			tempptr = heap[heap[tempptr].parent].children[childidx];
			// go BLM just in case
			while (heap[tempptr].children[0] != -1u) {
				tempptr = heap[tempptr].children[0];
				depth++;
				position += glm::vec3(1.) * pow(0.5, depth);
				childidx = 0u;
			}

		}
			// if we have reached the end of the level
		else {
			// step up until we find a sibling to the right
			while (childidx == 7) {
				position -= childDirection(childidx) * pow(0.5, depth);
				depth--;
				tempptr = heap[tempptr].parent;
				childidx = heap[tempptr].childidx;
			}
			// go to that sibling
			position -= childDirection(childidx) * pow(0.5, depth);
			childidx++;
			position += childDirection(childidx) * pow(0.5, depth);
			tempptr = heap[heap[tempptr].parent].children[childidx];
			// go BLM
			while (heap[tempptr].children[0] != -1u) {
				tempptr = heap[tempptr].children[0];
				depth++;
				position += glm::vec3(1.) * pow(0.5, depth);
				childidx = 0u;
			}
		}
	}
	nodeidx = tempptr;
	return position;
}

void Terrain::subdivide (uint32_t idx, Node* heap) {
	uint32_t freeptr = allocVoxel(heap);
	// uint32_t freeptr = heap[0].parent, nextfree = heap[freeptr].parent;
	for (uint8_t i = 0; i < 8; i++) {
		heap[idx].children[i] = freeptr + i;
		heap[freeptr + i].parent = idx;
		heap[freeptr + i].childidx = i;
		heap[freeptr + i].voxel = 0u;
		heap[freeptr + i].children[0] = -1u; // necessary??
		heap[freeptr + i].children[1] = 0u;
	}
	heap[0].childidx += 7;
	numleaves += 7;
	// updating first free record, and updating that block to have no previous and ensure it is marked free
	/*
	heap[0].parent = nextfree;
	heap[nextfree].childidx = -1u;
	heap[nextfree].voxel = -1u; // necessary??
				    // */
}

void Terrain::collapse (uint32_t parentidx, Node* heap) {
	uint32_t freeptr = heap[0].parent;
	while (freeptr < heap[parentidx].children[0]) {
		freeptr = heap[freeptr].parent;
	}
	numleaves -= 7;
	heap[heap[parentidx].children[0]].parent = heap[freeptr].parent;
	heap[heap[parentidx].children[0]].childidx = freeptr;
	heap[heap[parentidx].children[0]].voxel = -1u;
	// set relevant collapse flags for GPU to take over
	heap[heap[parentidx].children[0]].children[0] = -1u; // necessary??
	heap[heap[parentidx].children[0]].children[1] = 1u;
}

void Terrain::freeVoxel(uint32_t ptr, Node* heap) {
	// std::cout << "freeing block @ " << ptr << std::endl;
	// WE ARE NOT PROPERLY FREEING....SOMEHOW....
	// may have to do with the switch we made to passing in a ptr to the first in the block rather than its parent
	// or perhaps we're not inserting into the DLL correctly...
	uint32_t freeptr = heap[0].parent;
	while (freeptr < ptr) {
		freeptr = heap[freeptr].parent;
	}
	// std::cout << "bottom of loop @ " << ptr << std::endl;
	// technically the below two conditions should be equal, but i never trust my homebrew heap
	if (heap[0].parent == freeptr) heap[0].parent = ptr;
	if (heap[freeptr].childidx != -1u) heap[heap[freeptr].childidx].parent = ptr;
	heap[ptr].parent = freeptr;
	heap[ptr].childidx = heap[freeptr].childidx;
	heap[freeptr].childidx = ptr;
	/*
	heap[ptr].parent = heap[freeptr].parent;
	heap[ptr].childidx = freeptr;
	*/
	heap[ptr].voxel = -1u;
}

uint32_t Terrain::allocVoxel(Node* heap) {
	uint32_t freeptr = heap[0].parent, nextfree = heap[freeptr].parent;
	// updating first free record, and updating that block to have no previous and ensure it is marked free
	heap[0].parent = nextfree;
	heap[nextfree].childidx = -1u;
	heap[nextfree].voxel = -1u; // necessary??
	// std::cout << "allocd block @" << freeptr << std::endl;
	// std::cout << "nextfree @ " << nextfree << std::endl;
	return freeptr;
}

void Terrain::updateVoxels (glm::vec3 camerapos) {
	// no need to do anything if memory hasn't been used yet
	if (memoryavailable) return;

	setCompFIF(MAX_FRAMES_IN_FLIGHT + 1);

	updateNumLeaves();
	// dont forget to reset relevent counters!!
	// ideally we find some way to complete this function while gpu is not using these buffers
	// should be possible time-wise, perhaps difficult tho
	// also get rid of camerapos arg
	//
	// if we were getting really silly with it we could add a dynamic resizing here too lol
	void* tempptr, * heap;
	vkMapMemory(GraphicsHandler::vulkaninfo.logicaldevice,
		meminfostoragebuffer.memory,
		0u,
		sizeof(uint32_t) * (3 + ALLOC_INFO_BUF_SIZE),
		0,
		&tempptr);
	vkMapMemory(GraphicsHandler::vulkaninfo.logicaldevice,
		treestoragebuffer.memory,
		0u,
		numnodes * NODE_SIZE,
		0,
		&heap);

	Node* nodeheap = reinterpret_cast<Node*>(heap);
	uint32_t* meminfo = reinterpret_cast<uint32_t*>(tempptr);

	if (meminfo[3 + *meminfo] != -1u) std::cout << "excess memory should be reclaimed" << std::endl;

	uint32_t numtofree = *(meminfo + 1) - ALLOC_INFO_BUF_SIZE / 2, numtoalloc = *(meminfo + 2);
	if (numtofree != 0 || numtoalloc != 0) {
		std::cout << "freeing " << numtofree 
			<< ", allocing " << numtoalloc 
			<< " (out of " << numleaves << " total leaves)" << std::endl;
	}

	uint32_t ptr;
	for (ptr = 3 + ALLOC_INFO_BUF_SIZE / 2; ptr < numtofree + 3 + ALLOC_INFO_BUF_SIZE / 2; ptr++) {
		freeVoxel(meminfo[ptr], nodeheap);
		// meminfo[ptr] = -1u;
	}
	for (ptr = 3; ptr < numtoalloc + 3; ptr++) {
		meminfo[ptr] = allocVoxel(nodeheap);
	}
	if (numtoalloc != ALLOC_INFO_BUF_SIZE / 2 - 1) meminfo[ptr] = -1u;
	// dumpHeap(nodeheap, 9);

	*meminfo = 0; *(meminfo + 1) = ALLOC_INFO_BUF_SIZE / 2; *(meminfo + 2) = 0;

	// recalculating by iterating through DLL until we're sure its safe
	ptr = nodeheap[0].parent;
	nodeheapfreesize = 0u;
	// std::cout << "top of free size loop" << std::endl;
	uint32_t infinitycounter = 0;
	while (ptr != -1u && infinitycounter < 1024) {
		infinitycounter++;
		nodeheapfreesize++;
		ptr = nodeheap[ptr].parent;
	}
	if (ptr != -1u) {
		std::cout << "loop timeout" << std::endl;
		// dumpHeap(nodeheap, 25u);
	}
	// std::cout << "bottom of free size loop" << std::endl;

	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, meminfostoragebuffer.memory);
	vkUnmapMemory(GraphicsHandler::vulkaninfo.logicaldevice, treestoragebuffer.memory);
	
	memoryavailable = true;
}

void Terrain::dumpHeap (Node* heap, uint32_t n) {
	std::cout << heap[0].childidx << " leaves, first free block @" << heap[0].parent << std::endl;
	Node p;
	for (uint32_t i = 1; i < n; i++) {
		p = heap[i];
		if (p.voxel == -1u) {
			std::cout << p.childidx << " <-- " << i << " --> " << p.parent << std::endl;
		}
		else {
			std::cout << p.parent << std::endl;
			std::cout << i << std::endl;
			if (p.children[0] == -1u) std::cout << "x x x x x x x x" << std::endl;
			else {
				for (uint8_t c = 0; c < 8; c++) {
					std::cout << p.children[c] << " ";
				}
				std::cout << std::endl;
			}
		}
	}
}
