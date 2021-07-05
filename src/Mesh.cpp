//
// Created by Daniel Paavola on 5/30/21.
//

#include "Mesh.h"

Mesh::Mesh(){
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
//	bool queuefamilyindicesaresame=queuefamilyindices[0]==queuefamilyindices[1];
//	VkBufferCreateInfo buffercreateinfo{
//		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
//		nullptr,
//		0,
//		,
//		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//		queuefamilyindicesaresame?VK_SHARING_MODE_EXCLUSIVE:VK_SHARING_MODE_CONCURRENT,
//		queuefamilyindicesaresame?0:2,
//		queuefamilyindicesaresame?nullptr:&queuefamilyindices[0]
//	};
//	vkCreateBuffer(logicaldevice, )
}

Mesh::Mesh(const char*filepath, glm::vec3 p){
	position=p;
	loadOBJ(filepath);
	recalculateModelMatrix();
	shadowtype=RADIUS;
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
		unsigned int duplicate[3]={-1u, -1u, -1u};
		for(int y=0;y<3;y++){
			for(int z=0;z<combinations.size();z++){
				if(combinations[z]==glm::ivec3(vertexindices[x+y], uvindices[x+y], normalindices[x+y])){
					duplicate[y]=z;
				}
			}
		}
		unsigned int triindices[3]={-1u, -1u, -1u};
		for(int y=0;y<3;y++){
//			std::cout<<"loading... ("<<combinations.size()<<" combinations to check thru...)"<<std::endl;
			if(duplicate[y]==-1u){
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
			tris.push_back({{(unsigned int)(vertices.size()-3), (unsigned int)(vertices.size()-2), (unsigned int)(vertices.size()-1)}});
		}
	}
}