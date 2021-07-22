//
// Created by Daniel Paavola on 7/10/21.
//

#include "PhysicsHandler.h"

PhysicsHandler::PhysicsHandler(){
	currentland=nullptr;
	standingtri=nullptr;
	camera=nullptr;
	cameraPO={
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		0.0f
	};
}

PhysicsHandler::PhysicsHandler(Mesh*cl, Camera*c){
	currentland=cl;
	standingtri=nullptr;
	camera=c;
	cameraPO={
		camera->getPosition(),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		0.0f
	};
}

bool PhysicsHandler::edgeTest2D(glm::vec2 a, glm::vec2 b){
	//i think a rhs to lhs system change would really just mean swapping this inequality sign
	return (a.y*b.x)>(a.x*b.y);
}

bool PhysicsHandler::triIntersectionNoY(glm::vec3 tripoints[3], glm::vec3 q){
	return edgeTest2D(glm::vec2(tripoints[1].x-tripoints[0].x, tripoints[1].z-tripoints[0].z), glm::vec2(q.x-tripoints[0].x, q.z-tripoints[0].z))
		&&edgeTest2D(glm::vec2(tripoints[2].x-tripoints[1].x, tripoints[2].z-tripoints[1].z), glm::vec2(q.x-tripoints[1].x, q.z-tripoints[1].z))
		&&edgeTest2D(glm::vec2(tripoints[0].x-tripoints[2].x, tripoints[0].z-tripoints[2].z), glm::vec2(q.x-tripoints[2].x, q.z-tripoints[2].z));
}

void PhysicsHandler::updateStandingTri(){
	//this method doesn't factor in triangles stacked upon each other, but thats a bit more expensive, and not a likely problem
	//if it becomes a problem we'll just add a dollar-store depth buffer in here and check more tris
	const std::vector<Vertex> vertices=currentland->getVertices();
	if(standingtri==nullptr){
//		std::cout<<"pouring over all tris"<<std::endl;
		for(auto&t:*currentland->getTrisPtr()){
			//is allocation expensive?
			glm::vec3 temparray[3]={vertices[t.vertices[0]].position, vertices[t.vertices[1]].position, vertices[t.vertices[2]].position};
			if(triIntersectionNoY(temparray, cameraPO.position)){
				standingtri=&t;
				return;
			}
		}
//		std::cout<<"something's /super/ wrong, no triangle collisions were detected ANYWHERE..."<<std::endl;
		return;
	}
	glm::vec3 temparray[3]={vertices[standingtri->vertices[0]].position, vertices[standingtri->vertices[1]].position, vertices[standingtri->vertices[2]].position};
	//dont technically need below else after return
	if(triIntersectionNoY(temparray, cameraPO.position)) return;
	else{
//		std::cout<<"tri @ "<<standingtri<<" allegedly no longer collides, so we're looking thru adjacencies"<<std::endl;
		for(auto&t:standingtri->adjacencies){
			temparray[0]=vertices[t->vertices[0]].position; temparray[1]=vertices[t->vertices[1]].position; temparray[2]=vertices[t->vertices[2]].position;
			if(triIntersectionNoY(temparray, cameraPO.position)){
				standingtri=t;
				return;
			}
		}
	}
//	std::cout<<"something's /super/ wrong, no triangle collisions were detected ANYWHERE..."<<std::endl;
}

void PhysicsHandler::updateCameraPos(){
	cameraPO.position=camera->getPosition();
	updateStandingTri();

	float triintersectiony=
			(-standingtri->algebraicnormal.x*(cameraPO.position.x-currentland->getVertices()[standingtri->vertices[0]].position.x)
			 -standingtri->algebraicnormal.z*(cameraPO.position.z-currentland->getVertices()[standingtri->vertices[0]].position.z))
			 /standingtri->algebraicnormal.y+currentland->getVertices()[standingtri->vertices[0]].position.y;
//	std::cout<<triintersectiony<<std::endl;
//	if(triintersectiony<cameraPO.position.y) cameraPO.position.y-=0.1f;
//	if(triintersectiony>cameraPO.position.y) cameraPO.position.y+=0.1f;
	camera->setPosition(glm::vec3(camera->getPosition().x, triintersectiony+0.5f, camera->getPosition().z));
}