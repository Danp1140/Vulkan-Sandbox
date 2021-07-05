//
// Created by Daniel Paavola on 6/20/21.
//

#include "Grass.h"

Grass::Grass(){
	land=nullptr;
	blades=std::vector<Blade>();
}

Grass::Grass(Mesh*l, int count){
	land=l;
	//what's the best way to generate particle coordinates? simple random location could result in uneven spread
	//here's my first idea: one per face until count surpasses number of faces. then start duplicating
	//could have a number of functions, including true random, random per face, clumps, or even a custom function pointer
	std::vector<bool> spenttris=std::vector<bool>(l->getTris().size(), false);
	unsigned int counter, triid=-1u;
	srand(glfwGetTime());
	for(int x=0;x<count;x++){
		counter=0;
		triid=(unsigned int)rand()%l->getTris().size();
		while(true){
			if(spenttris[triid]){
				triid++;
				counter++;
				if(triid==l->getTris().size()) triid=0;
				if(counter==l->getTris().size()-1){
					std::fill(spenttris.begin(), spenttris.end(), false);
					continue;
				}
			}
			else{
				spenttris[triid]=true;
				break;
			}
		}
//		std::cout<<"generated tri index: "<<triid<<std::endl;
		blades.push_back({l->getVertices()[l->getTris()[triid].vertices[0]].position,
		                  {l->getVertices()[l->getTris()[triid].vertices[0]].position},
		                  0.1f, 1.0f, 1.0f, 3});
	}
//	blades=std::vector<Blade>(count, {glm::vec3(0, 0, 0), {glm::vec3(0, 0, 0)}, 1.0f, 1.0f, 1.0f, 1});
}

void Grass::draw(){
}