//
// Created by Daniel Paavola on 6/20/21.
//

#ifndef SANDBOX_GRASS_H
#define SANDBOX_GRASS_H

#define MAX_ARTICULATION_POINTS 4

#include "Mesh.h"

typedef struct{
	glm::vec3 position, vertices[MAX_ARTICULATION_POINTS];
	float width, length, stiffness;
	int articulationpoints;
	//assumed to be always growing in positive y direction
	//custom  stiffness variable: 0=>flaccid, 1=>rigid9
	void calculateVertices(){

	}
}Blade;

class Grass{
private:
	std::vector<Blade> blades;
	Mesh*land;
public:
	Grass();
	Grass(Mesh*l, int count);
	void draw();
};


#endif //SANDBOX_GRASS_H
