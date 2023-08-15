//
// Created by Daniel Paavola on 5/30/21.
//

#include "Mesh.h"

Mesh::Mesh (
		glm::vec3 p,
		glm::vec3 s,
		glm::quat r,
		uint32_t dir,
		uint32_t nir,
		uint32_t hir) : position(p), scale(s), rotation(r), dsmutex() {
	recalculateModelMatrix();
	initDescriptorSets();
	// hey man i really dont think we need a seperate buffer/memory per sci
	GraphicsHandler::VKSubInitUniformBuffer(
			&descriptorsets,
			0,
			sizeof(MeshUniformBuffer),
			1,
			&uniformbuffers,
			&uniformbuffermemories,
			GraphicsHandler::vulkaninfo.primarygraphicspipeline.objectdsl);
	texInit(dir, nir, hir);
	// TODO: make textures optional
	TextureHandler::generateTextures({diffusetexture, normaltexture, heighttexture}, TextureHandler::blankTexGenSet);
	rewriteTextureDescriptorSets();
}

Mesh::Mesh (
		const char* filepath,
		glm::vec3 p,
		glm::vec3 s,
		glm::quat r,
		uint32_t dir,
		uint32_t nir,
		uint32_t hir) : Mesh(p, s, r, dir, nir, hir) {
	loadOBJ(filepath);
}

Mesh::~Mesh () {
	if (vertexbuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(GraphicsHandler::vulkaninfo.logicaldevice, vertexbuffer, nullptr);
		vkFreeMemory(GraphicsHandler::vulkaninfo.logicaldevice, vertexbuffermemory, nullptr);
		vkDestroyBuffer(GraphicsHandler::vulkaninfo.logicaldevice, indexbuffer, nullptr);
		vkFreeMemory(GraphicsHandler::vulkaninfo.logicaldevice, indexbuffermemory, nullptr);
	}
	vkFreeDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
						 GraphicsHandler::vulkaninfo.descriptorpool,
						 GraphicsHandler::vulkaninfo.numswapchainimages,
						 descriptorsets);
	for (uint8_t scii = 0; scii < GraphicsHandler::vulkaninfo.numswapchainimages; scii++) {
		vkDestroyBuffer(GraphicsHandler::vulkaninfo.logicaldevice, uniformbuffers[scii], nullptr);
		vkFreeMemory(GraphicsHandler::vulkaninfo.logicaldevice, uniformbuffermemories[scii], nullptr);
	}
	delete[] descriptorsets;
	delete[] uniformbuffers;
	delete[] uniformbuffermemories;
}

void Mesh::texInit (uint32_t dir, uint32_t nir, uint32_t hir) {
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

/*
 * This method only works for convex polygons
 */
void Mesh::triangulatePolygon (std::vector<glm::vec3> v, std::vector<glm::vec3>& dst) {
	if (v.size() > 3) {
		for (uint8_t x = 0; x < 3; x++) dst.push_back(v[x]);
		std::vector<glm::vec3> temp = v;
		temp.erase(temp.begin() + 1);
		triangulatePolygon(temp, dst);
	} else {
		if (v.size() != 3) std::cout << "triangulation alg messed up\n";
		for (uint8_t x = 0; x < 3; x++) dst.push_back(v[x]);
	}
}

void Mesh::subdivide (uint8_t levels) {
	size_t trisizeinit;
	for (uint8_t l = 0; l < levels; l++) {
		trisizeinit = tris.size();
		for (size_t i = 0; i < trisizeinit; i++) {
			vertices.push_back({
									   (vertices[tris[0].vertexindices[1]].position +
										vertices[tris[0].vertexindices[0]].position) * 0.5,
									   (vertices[tris[0].vertexindices[1]].normal +
										vertices[tris[0].vertexindices[0]].normal) * 0.5,
									   (vertices[tris[0].vertexindices[1]].uv +
										vertices[tris[0].vertexindices[0]].uv) * 0.5
							   });
			vertices.push_back({
									   (vertices[tris[0].vertexindices[2]].position +
										vertices[tris[0].vertexindices[1]].position) * 0.5,
									   (vertices[tris[0].vertexindices[2]].normal +
										vertices[tris[0].vertexindices[1]].normal) * 0.5,
									   (vertices[tris[0].vertexindices[2]].uv +
										vertices[tris[0].vertexindices[1]].uv) * 0.5
							   });
			vertices.push_back({
									   (vertices[tris[0].vertexindices[0]].position +
										vertices[tris[0].vertexindices[2]].position) * 0.5,
									   (vertices[tris[0].vertexindices[0]].normal +
										vertices[tris[0].vertexindices[2]].normal) * 0.5,
									   (vertices[tris[0].vertexindices[0]].uv +
										vertices[tris[0].vertexindices[2]].uv) * 0.5
							   });
			tris.push_back({});
			tris.back().vertexindices[0] = tris[0].vertexindices[0];
			tris.back().vertexindices[1] = vertices.size() - 3;
			tris.back().vertexindices[2] = vertices.size() - 1;
			tris.push_back({});
			tris.back().vertexindices[0] = vertices.size() - 3;
			tris.back().vertexindices[1] = tris[0].vertexindices[1];
			tris.back().vertexindices[2] = vertices.size() - 2;
			tris.push_back({});
			tris.back().vertexindices[0] = vertices.size() - 1;
			tris.back().vertexindices[1] = vertices.size() - 2;
			tris.back().vertexindices[2] = tris[0].vertexindices[2];
			tris.push_back({});
			tris.back().vertexindices[0] = vertices.size() - 3;
			tris.back().vertexindices[1] = vertices.size() - 2;
			tris.back().vertexindices[2] = vertices.size() - 1;
//			tris.erase(tris.begin()+i);
			tris.erase(tris.begin());
		}
	}
	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(vertices,
													   tris,
													   &vertexbuffer,
													   &vertexbuffermemory,
													   &indexbuffer,
													   &indexbuffermemory);
}

void Mesh::generateSmoothVertexNormals () {
	glm::vec3 total;
	size_t numtris;
	for (Vertex& v1: vertices) {
		total = glm::vec3(0.);
		numtris = 0u;
		for (const Tri& t: tris) {
			for (const Vertex* v2: t.vertices) {
				if (&v1 == v2) {
					total += t.algebraicnormal;
					numtris++;
				}
			}
		}
		v1.normal = total / numtris;
	}
}

void Mesh::initDescriptorSets () {
	VkDescriptorSetLayout layoutstemp[MAX_FRAMES_IN_FLIGHT];
	for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++)
		layoutstemp[x] = GraphicsHandler::vulkaninfo.primarygraphicspipeline.objectdsl;
	if (descriptorsets)
		vkFreeDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
							 GraphicsHandler::vulkaninfo.descriptorpool,
							 MAX_FRAMES_IN_FLIGHT,
							 descriptorsets);
	delete[] descriptorsets;
	descriptorsets = new VkDescriptorSet[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSetAllocateInfo descriptorsetallocinfo {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			nullptr,
			GraphicsHandler::vulkaninfo.descriptorpool,
			MAX_FRAMES_IN_FLIGHT,
			&layoutstemp[0]
	};
	std::cout << string_VkResult(vkAllocateDescriptorSets(GraphicsHandler::vulkaninfo.logicaldevice,
														  &descriptorsetallocinfo,
														  &descriptorsets[0])) << std::endl;
}

void Mesh::rewriteTextureDescriptorSets () {
	VkDescriptorImageInfo imginfo = diffusetexture.getDescriptorImageInfo();
	for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		VkWriteDescriptorSet write {
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
	imginfo = normaltexture.getDescriptorImageInfo();
	for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		VkWriteDescriptorSet write {
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
	imginfo = heighttexture.getDescriptorImageInfo();
	for (uint32_t x = 0; x < MAX_FRAMES_IN_FLIGHT; x++) {
		VkWriteDescriptorSet write {
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

void Mesh::recordDraw (cbRecData data, VkCommandBuffer& cb) {
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
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
			VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PrimaryGraphicsPushConstants),
			data.pushconstantdata);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipelinelayout,
			0,
			1, &data.scenedescriptorset,
			0, nullptr);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipelinelayout,
			1,
			1,
			&data.descriptorset,
			0, nullptr);
	VkDeviceSize offsettemp = 0u;
	vkCmdBindVertexBuffers(
			cb,
			0,
			1,
			&data.vertexbuffer,
			&offsettemp);
	vkCmdBindIndexBuffer(
			cb,
			data.indexbuffer,
			0,
			VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(
			cb,
			data.numtris * 3,
			1,
			0,
			0,
			0);
	vkEndCommandBuffer(cb);
}

void Mesh::recordShadowDraw (cbRecData data, VkCommandBuffer& cb) {
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
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ShadowmapPushConstants),
			data.pushconstantdata);
	vkCmdBindDescriptorSets(
			cb,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data.pipeline.pipelinelayout,
			1,
			1, &data.descriptorset,
			0, nullptr);
	VkDeviceSize offsettemp = 0u;
	vkCmdBindVertexBuffers(
			cb,
			0,
			1,
			&data.vertexbuffer,
			&offsettemp);
	vkCmdBindIndexBuffer(
			cb,
			data.indexbuffer,
			0,
			VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(
			cb,
			data.numtris * 3,
			1,
			0,
			0,
			0);
	vkEndCommandBuffer(cb);
}

void
Mesh::generateSteppeMesh (std::vector<glm::vec3> area, std::vector<std::vector<glm::vec3>> waters, double seed) {
	vertices = std::vector<Vertex>();
	tris = std::vector<Tri>();
	std::vector<glm::vec3> vertpostemp;
	triangulatePolygon(area, vertpostemp);
	for (uint32_t i = 0; i < vertpostemp.size(); i++) {
		vertices.push_back(
				{vertpostemp[i], glm::vec3(0., 1., 0.), glm::vec2(vertpostemp[i].x, vertpostemp[i].z)});
		if (i % 3 == 0) {
			tris.push_back({});
			tris.back().vertexindices[0] = i;
			tris.back().vertexindices[1] = i + 1;
			tris.back().vertexindices[2] = i + 2;
		}
	}
	subdivide(4);
	cleanUpVertsAndTris();
	float depthtemp = -2.5f, mindist, disttemp;
	for (std::vector<glm::vec3>& w: waters) {
		for (Vertex& v: vertices) {
			mindist = 99999999.9f;
			for (size_t i = 1; i + 2 < w.size(); i++) {
				for (float_t u = 0.f; u <= 1.f; u += 0.01f) {
					disttemp = glm::distance(PhysicsHandler::catmullRomSplineAtMatrified(w, i, 0.5f, u),
											 v.position);
					if (disttemp < mindist) mindist = disttemp;
				}
			}
			v.position.y = ((mindist > -depthtemp) ? 0. : (mindist + depthtemp));
		}
	}
	cleanUpVertsAndTris();
	generateSmoothVertexNormals();
	// shouldn't /technically/ be using this function, should instead create an update function in GH
	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(
			vertices,
			tris,
			&vertexbuffer,
			&vertexbuffermemory,
			&indexbuffer,
			&indexbuffermemory);
}

void Mesh::recalculateModelMatrix () {
	uniformbufferdata.modelmatrix = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
}

void Mesh::setPosition (glm::vec3 p) {
	position = p;
	recalculateModelMatrix();
}

void Mesh::setRotation (glm::quat r) {
	rotation = r;
	recalculateModelMatrix();
}

void Mesh::setScale (glm::vec3 s) {
	scale = s;
	recalculateModelMatrix();
}

MeshUniformBuffer Mesh::getUniformBufferData () {
	uniformbufferdata.diffuseuvmatrix = diffusetexture.uvmatrix;
	uniformbufferdata.normaluvmatrix = normaltexture.uvmatrix;
	uniformbufferdata.heightuvmatrix = heighttexture.uvmatrix;
	return uniformbufferdata;
}

void Mesh::cleanUpVertsAndTris () {
	uint32_t initverttotal = vertices.size();
	//any efficiency possible with breaks and stuff??
#ifndef NO_LOADING_BARS
	std::cout << "removing duplicates: " << ANSI_GREEN_FORE << std::endl;
	uint16_t dupecounter = 0;
#endif
	for (int x = 0; x < vertices.size(); x++) {
		for (int y = 0; y < x; y++) {
			if (y != x && vertices[x] == vertices[y]) {
				for (int z = 0; z < tris.size(); z++) {
					for (int w = 0; w < 3; w++) {
						if (tris[z].vertexindices[w] == x) tris[z].vertexindices[w] = y;
						else if (tris[z].vertexindices[w] > x) tris[z].vertexindices[w]--;
					}
				}
				if (x < vertices.size()) vertices.erase(vertices.begin() + x);
#ifndef NO_LOADING_BARS
				dupecounter++;
				std::cout << '\r';
				for (uint8_t bars = 0; bars < 50; bars++) {
					if (float(x + 1) / float(vertices.size()) * 100. >= float(bars) * 2.) std::cout << "\u2588";
					else std::cout << ' ';
				}
#endif
			}
		}
	}
#ifndef NO_LOADING_BARS
	std::cout << ANSI_RESET_FORE << std::endl;
	std::cout << dupecounter << " duplicates removed! (" << (float(dupecounter) / float(initverttotal) * 100.0f)
			  << " percent size decrease!!!)" << std::endl;


	std::cout << "finding tri adjacencies: " << ANSI_GREEN_FORE << std::endl;
#endif
	bool adjacencyfound;
	for (int x = 0; x < tris.size(); x++) {
		for (int y = 0; y < tris.size(); y++) {
			if (x != y) {
				adjacencyfound = false;
				for (int z = 0; z < 3; z++) {
					for (int w = 0; w < 3; w++) {
						if (vertices[tris[x].vertexindices[z]].position ==
							vertices[tris[y].vertexindices[w]].position) {
							tris[x].adjacencies.push_back(&tris[y]);
							adjacencyfound = true;
							break;
						}
					}
					if (adjacencyfound) break;
				}
			}
		}
#ifndef NO_LOADING_BARS
		std::cout << '\r';
		for (uint8_t bars = 0; bars < 50; bars++) {
			if (float(x + 1) / float(tris.size()) * 100. >= float(bars * 2.)) std::cout << "\u2588";
			else std::cout << ' ';
		}
#endif
	}
#ifndef NO_LOADING_BARS
	std::cout << ANSI_RESET_FORE << std::endl;
#endif

	for (uint32_t x = 0; x < tris.size(); x++) {
		for (uint16_t y = 0; y < 3; y++) {
			tris[x].vertices[y] = &vertices[tris[x].vertexindices[y]];
#ifndef NO_LOADING_BARS
			std::cout << "\rgiving tris vertex addresses (tri " << (x + 1) << " of " << tris.size() << ')';
#endif
		}
	}
#ifndef NO_LOADING_BARS
	std::cout << std::endl;
#endif

	for (auto& t: tris) {
		t.algebraicnormal = glm::normalize(
				glm::cross(vertices[t.vertexindices[1]].position - vertices[t.vertexindices[0]].position,
						   vertices[t.vertexindices[2]].position - vertices[t.vertexindices[0]].position));
	}

	for (auto& v: vertices) {
		for (uint8_t x = 0; x < 3; x++) {
			if (v.position[x] < min[x]) min[x] = v.position[x];
			if (v.position[x] > max[x]) max[x] = v.position[x];
		}
	}

	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(vertices,
													   tris,
													   &vertexbuffer,
													   &vertexbuffermemory,
													   &indexbuffer,
													   &indexbuffermemory);
}

void Mesh::loadOBJ (const char* filepath) {
	tris = std::vector<Tri>();
	vertices = std::vector<Vertex>();
	std::vector<unsigned int> vertexindices, uvindices, normalindices;
	std::vector<glm::vec3> vertextemps, normaltemps;
	std::vector<glm::vec2> uvtemps;
	FILE* obj = fopen(filepath, "r");
	if (obj == nullptr) {
		std::cout << "Loading OBJ Failure (" << filepath << ")\n";
		return;
	}
	while (true) {
		char lineheader[128];//128?
		int res = fscanf(obj, "%s", lineheader);
		if (res == EOF) {break;}
		if (strcmp(lineheader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(obj, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertextemps.push_back(vertex);
		} else if (strcmp(lineheader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(obj, "%f %f\n", &uv.x, &uv.y);
			uvtemps.push_back(uv);
		} else if (strcmp(lineheader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(obj, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			normaltemps.push_back(normal);
		} else if (strcmp(lineheader, "f") == 0) {
			unsigned int vertidx[3], uvidx[3], normidx[3];
			int matches = fscanf(obj, "%d/%d/%d %d/%d/%d %d/%d/%d", &vertidx[0], &uvidx[0], &normidx[0],
								 &vertidx[1], &uvidx[1], &normidx[1],
								 &vertidx[2], &uvidx[2], &normidx[2]);
			if (matches != 9) {
				std::cout << "OBJ Format Failure (" << filepath << ")\n";
				return;
			}
			for (auto& v: vertidx) vertexindices.push_back(v - 1);
			for (auto& n: normidx) normalindices.push_back(n - 1);
			for (auto& u: uvidx) uvindices.push_back(u - 1);
		}
	}

	for (uint16_t x = 0; x < vertexindices.size() / 3; x++) {
		tris.push_back({});
		for (uint16_t y = 0; y < 3; y++) {
			std::cout << "\rassembling all vertexindices and triangles (" << (x * 3 + y + 1) << '/'
					  << vertexindices.size() << ')';
			vertices.push_back({vertextemps[vertexindices[3 * x + y]], normaltemps[normalindices[3 * x + y]],
								uvtemps[uvindices[3 * x + y]]});
			tris[tris.size() - 1].vertexindices[y] = 3 * x + y;
		}
	}
	std::cout << std::endl;

	cleanUpVertsAndTris();
}

void Mesh::makeIntoIcosphere () {
	loadOBJ(WORKING_DIRECTORY "/resources/objs/icosphere.obj");
}

void Mesh::makeIntoCube () {
	loadOBJ(WORKING_DIRECTORY "/resources/objs/fuckingcube.obj");
}

Mesh* Mesh::generateBoulder (RockType type, glm::vec3 scale, uint seed) {
	Mesh* result = new Mesh(glm::vec3(0.f), glm::vec3(1.f), glm::quat(0.f, 0.f, 0.f, 1.f), 1024, 1024, 1024);
	std::default_random_engine randeng(seed);
	std::uniform_real_distribution<float_t> unidist {};
	if (type == ROCK_TYPE_GRANITE) {
		glm::mat4 transform(1.f);
		transform = glm::translate(glm::vec3(0.f, -1.f, 0.f));
//		transform = glm::scale(glm::vec3(5.f));
		result->makeIntoCube();
//		result->subdivide(5);
//		result->proportionalTransform(transform, glm::vec3(0.f, -1.f, 0.f), 0.5f);
		result->transform(transform);

//		TextureInfo* texdsts[3] {result->getDiffuseTexturePtr(),
//								 result->getNormalTexturePtr(),
//								 result->getHeightTexturePtr()};
//		TextureHandler::generateGraniteTextures(&texdsts[0], 3);
//		result->getDiffuseTexturePtr()->setUVScale(glm::vec2(100.f, 100.f));
//		TextureHandler::generateNewSystemTextures({result->diffusetexture,
//												   result->normaltexture,
//												   result->heighttexture});
		TextureHandler::generateTextures({result->diffusetexture,
										  result->normaltexture,
										  result->heighttexture},
										 TextureHandler::colorfulMarbleTexGenSet);
		result->rewriteTextureDescriptorSets();
	}
	return result;
}

void Mesh::transform (glm::mat4 m) {
	glm::vec4 temp;
	for (auto& v: vertices) {
		temp = m * glm::vec4(v.position.x, v.position.y, v.position.z, 1.f);
		v.position = temp / temp.w;
	}
	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(vertices,
													   tris,
													   &vertexbuffer,
													   &vertexbuffermemory,
													   &indexbuffer,
													   &indexbuffermemory);
}

void Mesh::proportionalTransform (glm::mat4 m, glm::vec3 o, float_t r) {
	glm::vec4 temp;
	float coeff;
	for (auto& v: vertices) {
//		coeff = std::max(1.f - glm::distance(o, v.position) / r, 0.f);	// linear
//		coeff = std::max(exp(-pow(glm::distance(o, v.position), 2.f)) / r, 0.f); // gaussian
		coeff = std::max(1.f - pow(glm::distance(o, v.position) / r, 2.f), 0.f); // quadratic
		temp = glm::mix(glm::mat4(1.f), m, coeff) * glm::vec4(v.position.x, v.position.y, v.position.z, 1.f);
		v.position = temp / temp.w;
	}
	GraphicsHandler::VKHelperInitVertexAndIndexBuffers(vertices,
													   tris,
													   &vertexbuffer,
													   &vertexbuffermemory,
													   &indexbuffer,
													   &indexbuffermemory);
}