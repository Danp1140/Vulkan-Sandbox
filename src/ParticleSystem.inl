template<class T>
ParticleSystem<T>::ParticleSystem (
		ParticleType t, uint32_t nparticles, DistributionType disttype, DistributionData distdata):
		Mesh(glm::vec3(0.0, 0.0, 0.0)), distributiondata(distdata) {
	type = t;
	numparticles = nparticles;
	variations = 4;
	distributiontype = disttype;
	particles = std::vector<T>();

	if (distdata.mesh != nullptr) position = distdata.mesh->getPosition();

	distributeParticles();
	vertices.push_back({
		position + glm::vec3(0.01, 0.0, 0.0),
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec2(1.0, 0.0)
	});
	vertices.push_back({
		position + glm::vec3(0.0, 0.5, 0.0),
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec2(0.5, 1.0)
	});
	vertices.push_back({
		position + glm::vec3(-0.01, 0.0, 0.0),
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec2(0.0, 0.0)
	});
	tris.push_back({
		{0, 0, 0},
		{&vertices[vertices.size() - 3], &vertices[vertices.size() - 2],
		&vertices[vertices.size() - 1]
		}, std::vector<Tri*>(),
		glm::vec3(0.0, 0.0, 1.0)
	});
	tris.back().vertexindices[0] = vertices.size() - 3;
	tris.back().vertexindices[1] = vertices.size() - 2;
	tris.back().vertexindices[2] = vertices.size() - 1;

	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(
		vertices,
		tris,
		&vertexbuffer,
		&vertexbuffermemory,
		&indexbuffer,
		&indexbuffermemory);
	initDescriptorSets(GraphicsHandler::vulkaninfo.grassgraphicspipeline.objectdsl);

	storagebuffer.elemsize = sizeof(glm::mat4);
	storagebuffer.numelems = numparticles;
	GraphicsHandler::VKSubInitStorageBuffer(storagebuffer);

	glm::mat4 matstemp[numparticles];
	for (uint32_t n = 0; n < numparticles; n++) matstemp[n] = particles[n].modelmatrix;
	GraphicsHandler::VKHelperUpdateStorageBuffer(storagebuffer, static_cast<void*>(&matstemp[0]));
}

template<class T>
void ParticleSystem<T>::distributeParticles () {
	if (distributiontype == ENFORCED_UNIFORM_ON_MESH) {
		std::default_random_engine randeng = std::default_random_engine();
		std::uniform_real_distribution<float> unirealdist;
		float barya, baryb, baryc;
		Tri tritemp;
		for (uint32_t x = 0; x < numparticles; x++) {
			//probably not truly random
			unirealdist = std::uniform_real_distribution<float>(0.0, 1.0);
			barya = unirealdist(randeng);
			unirealdist = std::uniform_real_distribution<float>(0.0, 1. - barya);
			baryb = unirealdist(randeng);
			unirealdist = std::uniform_real_distribution<float>(0.0, 1. - baryb - barya);
			baryc = unirealdist(randeng);
			tritemp = distributiondata.mesh->getTris()[x % distributiondata.mesh->getTris().size()];
			particles.push_back(T(tritemp.vertices[0]->position * barya
								  + tritemp.vertices[1]->position * baryb
								  + tritemp.vertices[2]->position * baryc));
//			std::cout<<"particle pos: "<<SHIFT_TRIPLE(particles.back().position)<<'\n';
		}
	}
}

template<class T>
void ParticleSystem<T>::createPipeline () {
	VkDescriptorSetLayoutBinding objectbinding {
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			nullptr
	};
	VkDescriptorSetLayoutCreateInfo descsetlayoutcreateinfos[2] {
			scenedslcreateinfo, {
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					nullptr,
					0,
					1,
					&objectbinding
			}};
	VkVertexInputBindingDescription vertinbindingdescs {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdescs[2] {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}
	};


	PipelineInitInfo pii = {};
	pii.stages = VK_SHADER_STAGE_VERTEX_BIT
				 | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
				 | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
				 | VK_SHADER_STAGE_FRAGMENT_BIT;
	pii.shaderfilepathprefix = "grass";
	pii.descsetlayoutcreateinfos = &descsetlayoutcreateinfos[0];
	pii.pushconstantrange = {VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, sizeof(glm::mat4)};
	pii.vertexinputstatecreateinfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			1,
			&vertinbindingdescs,
			2,
			&vertinattribdescs[0]
	};
	pii.depthtest = true;

	GraphicsHandler::VKSubInitPipeline(&GraphicsHandler::vulkaninfo.grassgraphicspipeline, pii);
}

template<class T>
void ParticleSystem<T>::recordDraw (uint8_t fifindex, uint8_t sciindex, VkDescriptorSet* sceneds) const {
//	VkCommandBufferInheritanceInfo cmdbufinherinfo{
//			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
//			nullptr,
//			GraphicsHandler::vulkaninfo.primaryrenderpass,
//			0,
//			GraphicsHandler::vulkaninfo.primaryframebuffers[sciindex],
//			VK_FALSE,
//			0,
//			0
//	};
//	VkCommandBufferBeginInfo cmdbufbegininfo{
//			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//			nullptr,
//			VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
//			&cmdbufinherinfo
//	};
//	vkBeginCommandBuffer(commandbuffers[fifindex], &cmdbufbegininfo);
//	vkCmdBindPipeline(
//			commandbuffers[fifindex],
//			VK_PIPELINE_BIND_POINT_GRAPHICS,
//			GraphicsHandler::vulkaninfo.grassgraphicspipeline.pipeline);
//	vkCmdPushConstants(
//			commandbuffers[fifindex],
//			GraphicsHandler::vulkaninfo.grassgraphicspipeline.pipelinelayout,
//			VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
//			0,
//			sizeof(glm::mat4),
//			&GraphicsHandler::vulkaninfo.grasspushconstants);
//	vkCmdBindDescriptorSets(
//			commandbuffers[fifindex],
//			VK_PIPELINE_BIND_POINT_GRAPHICS,
//			GraphicsHandler::vulkaninfo.grassgraphicspipeline.pipelinelayout,
//			1,
//			1,
//			&descriptorsets[sciindex],
//			0, nullptr);
//	VkDeviceSize offsettemp = 0u;
//	vkCmdBindVertexBuffers(
//			commandbuffers[fifindex],
//			0,
//			1,
//			&vertexbuffer,
//			&offsettemp);
//	vkCmdBindIndexBuffer(
//			commandbuffers[fifindex],
//			indexbuffer,
//			0,
//			VK_INDEX_TYPE_UINT16);
//	vkCmdDrawIndexed(
//			commandbuffers[fifindex],
//			tris.size() * 3,
//			numparticles,
//			0,
//			0,
//			0);
//	vkEndCommandBuffer(commandbuffers[fifindex]);
}
