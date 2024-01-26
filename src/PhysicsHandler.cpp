//
// Created by Daniel Paavola on 7/10/21.
//

#include "PhysicsHandler.h"

PhysicsHandler::PhysicsHandler () {
	standingtri = nullptr;
	standinguv = glm::vec2(0.0f, 0.0f);
	camera = nullptr;
	cameraPO = {
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			0.0f
	};
	lasttime = std::chrono::high_resolution_clock::now();
}

PhysicsHandler::PhysicsHandler (Camera*c){
	standingtri = nullptr;
	standinguv = glm::vec2(0.0f, 0.0f);
	camera = c;
	cameraPO = {
			camera->getPosition(),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			0.0f
	};
	lasttime = std::chrono::high_resolution_clock::now();
}

bool PhysicsHandler::triIntersectionNoY (glm::vec3 tripoints[3], glm::vec3 q) {
	return edgeTest2D(glm::vec2(tripoints[1].x - tripoints[0].x, tripoints[1].z - tripoints[0].z),
					  glm::vec2(q.x - tripoints[0].x, q.z - tripoints[0].z))
		   && edgeTest2D(glm::vec2(tripoints[2].x - tripoints[1].x, tripoints[2].z - tripoints[1].z),
						 glm::vec2(q.x - tripoints[1].x, q.z - tripoints[1].z))
		   && edgeTest2D(glm::vec2(tripoints[0].x - tripoints[2].x, tripoints[0].z - tripoints[2].z),
						 glm::vec2(q.x - tripoints[2].x, q.z - tripoints[2].z));
}

bool PhysicsHandler::triIntersection2D (glm::vec2 p[3], glm::vec2 q) {
	return edgeTest2D(p[1] - p[0], q - p[0]) && edgeTest2D(p[2] - p[1], q - p[1]) && edgeTest2D(p[0] - p[2], q - p[2]);
}

bool PhysicsHandler::cylinderTriIntersection (Tri tri, glm::vec3 cylinderorigin, float r, float h) {
	glm::vec3 overtices[3] = {tri.vertices[0]->position - cylinderorigin, tri.vertices[1]->position - cylinderorigin,
			tri.vertices[2]->position - cylinderorigin};
	//gotta do this check first cause other tests assume vertices exist only outside the cylinder
	for (uint16_t x = 0; x < 3; x++) if (pow(overtices[x].x, 2) + pow(overtices[x].z, 2) <= pow(r, 2)) return true;
	if (tri.algebraicnormal.y == 0.0f) {
		//is this efficient? we could just do a bunch more logic ops instead of some additions/multiplications
		float tzero, th;
		for (uint16_t x = 0; x < 3; x++) {
			tzero = -overtices[x].y / (overtices[(x + 1) % 3].y - overtices[x].y);
			th = tzero + h / (overtices[(x + 1) % 3].y - overtices[x].y);
			if ((tzero > 1.0f && th > 1.0f) || (tzero < 0.0f && th < 0.0f)) return false;
		}
		float m = (overtices[1].z - overtices[0].z) / (overtices[1].x - overtices[0].x);
		float b = overtices[0].z - m * (overtices[0].x);
		return abs(b / atan(m)) <= r;
	}
	if (tri.algebraicnormal.y == 1.0f) {
		if (tri.vertices[0]->position.y < h + cylinderorigin.y && tri.vertices[0]->position.y > cylinderorigin.y) {
			float m, b, minr, theta;
			for (uint16_t x = 0; x < 3; x++) {
				m = (overtices[(x + 1) % 3].z - overtices[x].z) / (overtices[(x + 1) % 3].x - overtices[x].x);
				b = overtices[x].z - m * (overtices[x].x);
				theta = atan(-m);
				minr = b / (cos(theta) - m * sin(theta));
				if (abs(minr) <= r) return true;
				//if this works out, we may only have to compute x
				glm::vec2 mincartesian = glm::vec2(minr * sin(theta), minr * cos(theta));     //this vector is <x, z>
				float t = (mincartesian.x - overtices[x].x) / (overtices[(x + 1) % 3].x - overtices[x].x);
				if (t < 0.0f && t > 1.0f) return false;
			}
			return true;
		}
		return false;
	}


	//function unfinished, did this to shut up clang
	return false;
}

glm::vec3 PhysicsHandler::catmullRomSplineAtMatrified (
		const std::vector<glm::vec3>&controlpoints,
		size_t p1index,
		float_t tau,
		float_t u){
	glm::vec4 uT = glm::vec4(1., u, pow(u, 2.), pow(u, 3.));
	glm::mat4 M = glm::transpose(glm::mat4(0., 1., 0., 0.,
	                                       - tau, 0., tau, 0.,
	                                       2. * tau, tau - 3., 3. - 2. * tau, - tau,
	                                       - tau, 2. - tau, tau - 2., tau));
	glm::mat3x4 p = glm::transpose(glm::mat4x3(controlpoints[p1index - 1],
	                                           controlpoints[p1index],
	                                           controlpoints[p1index + 1],
	                                           controlpoints[p1index + 2]));
	return uT * M * p;
}

void PhysicsHandler::updateStandingTri (const std::vector<Tri>&land){
	//this method doesn't factor in triangles stacked upon each other, but thats a bit more expensive, and not a likely problem
	//if it becomes a problem we'll just add a dollar-store depth buffer in here and check more tris
	if (standingtri == nullptr){
		for (const Tri&t:land){
			//is allocation expensive? we could place it outside the for loop, or even outside the if checks
			glm::vec3 temparray[3] = {t.vertices[0]->position, t.vertices[1]->position, t.vertices[2]->position};
			if (triIntersectionNoY(temparray, cameraPO.position)) {
				standingtri = &t;
				return;
			}
		}
		return;
	}
	glm::vec3 temparray[3] = {standingtri->vertices[0]->position, standingtri->vertices[1]->position,
			standingtri->vertices[2]->position};
	//is there a more efficient calculation method that involves detecting edge cross via difference in camera position?
	if (triIntersectionNoY(temparray, cameraPO.position)) return;
	for (auto& t: standingtri->adjacencies) {
		temparray[0] = t->vertices[0]->position;
		temparray[1] = t->vertices[1]->position;
		temparray[2] = t->vertices[2]->position;
		if (triIntersectionNoY(temparray, cameraPO.position)) {
			standingtri = t;
			return;
		}
	}
	standingtri = nullptr;
}

void PhysicsHandler::updateCameraPos (const std::vector<Tri>&land){
	//need to take a step back and clean this up sometime
	//probably some early escapes we could use
	if (standingtri != nullptr){
		//do we /really/ need to calculate this again???
		//could probably store this in physicshander or physicsobject
		float triintersectiony =
				(-standingtri->algebraicnormal.x *
				 (cameraPO.position.x - standingtri->vertices[0]->position.x)
				 - standingtri->algebraicnormal.z *
				   (cameraPO.position.z - standingtri->vertices[0]->position.z))
				/ standingtri->algebraicnormal.y + standingtri->vertices[0]->position.y;
		glm::vec3 collision = glm::vec3(camera->getPosition().x, triintersectiony, camera->getPosition().z);
		//notably, this method disallows the preservation of walking momentum in air
		//to remedy i think we gotta store the value outside of the function's scope (up in PhysicsHandler somewhere...)
		glm::vec3 walkingvelocity = glm::vec3(0.0f), gravityacceleration = glm::vec3(0.0f);
		if (cameraPO.position.y > triintersectiony) gravityacceleration.y = PHYSICS_HANDLER_GRAVITY;
		if (cameraPO.position.y < triintersectiony + STEP_HEIGHT) {
			//should check where vectors are normalized to avoid redundant normalizing
			//itd be even cooler if we restricted total movement speed, cause right now diagonals are just twice as fast lol
			glm::vec3 right = glm::vec3(-camera->getForward().z, 0.0f, camera->getForward().x);
			if (glfwGetKey(GraphicsHandler::vulkaninfo.window, GLFW_KEY_SPACE) == GLFW_PRESS
				|| glfwGetKey(GraphicsHandler::vulkaninfo.window, GLFW_KEY_SPACE) == GLFW_REPEAT)
				cameraPO.acceleration += glm::vec3(0.0f, 10.0f, 0.0f);
			KeyInfo keytemp = GraphicsHandler::keyvalues.find(GLFW_KEY_W)->second;
			if (keytemp.currentvalue == GLFW_PRESS || keytemp.currentvalue == GLFW_REPEAT) {
				walkingvelocity += glm::normalize(glm::cross(standingtri->algebraicnormal, right));
			}
			keytemp = GraphicsHandler::keyvalues.find(GLFW_KEY_A)->second;
			if (keytemp.currentvalue == GLFW_PRESS || keytemp.currentvalue == GLFW_REPEAT) {
				walkingvelocity += glm::normalize(glm::cross(standingtri->algebraicnormal, camera->getForward()));
			}
			keytemp = GraphicsHandler::keyvalues.find(GLFW_KEY_S)->second;
			if (keytemp.currentvalue == GLFW_PRESS || keytemp.currentvalue == GLFW_REPEAT) {
				walkingvelocity += glm::normalize(glm::cross(standingtri->algebraicnormal, -right));
			}
			keytemp = GraphicsHandler::keyvalues.find(GLFW_KEY_D)->second;
			if (keytemp.currentvalue == GLFW_PRESS || keytemp.currentvalue == GLFW_REPEAT) {
				walkingvelocity += glm::normalize(glm::cross(standingtri->algebraicnormal, -camera->getForward()));
			}
		} else if (cameraPO.position.y > triintersectiony + STEP_HEIGHT) {
			cameraPO.acceleration = glm::vec3(0.0f);
		}
		cameraPO.position += (cameraPO.velocity * dt + walkingvelocity * dt * 5.0f) +
							 (0.5f * (cameraPO.acceleration + gravityacceleration) * pow(dt, 2));
		cameraPO.velocity += (cameraPO.acceleration + gravityacceleration) * dt;

		updateStandingTri(land);
		//redundant computation???
		if (standingtri != nullptr) {
			triintersectiony =
					(-standingtri->algebraicnormal.x *
					 (cameraPO.position.x - standingtri->vertices[0]->position.x)
					 - standingtri->algebraicnormal.z *
					   (cameraPO.position.z - standingtri->vertices[0]->position.z))
					/ standingtri->algebraicnormal.y + standingtri->vertices[0]->position.y;
			//maybe use cameraPO for this one instead of camera->getPosition()
			collision = glm::vec3(cameraPO.position.x, triintersectiony, cameraPO.position.z);
			if (cameraPO.position.y < triintersectiony) {
				cameraPO.acceleration = glm::vec3(0.0f);
				cameraPO.velocity = glm::vec3(0.0f);
				cameraPO.position.y = triintersectiony;
			}

			glm::vec3 barycentricvalues;
			for (uint32_t x = 0; x < 3; x++) {
				//very likely a more efficient, but more manual way to do this
				//would still like to figure out why passing collision here didn't work, but cameraPO did
				//and furthermore why they aren't actually equal, despite their y components seeming to be
				barycentricvalues[x] = glm::length(
						glm::cross(cameraPO.position - standingtri->vertices[(x + 1) % 3]->position,
								   standingtri->vertices[(x + 2) % 3]->position -
								   standingtri->vertices[(x + 1) % 3]->position));
			}
			barycentricvalues /= (barycentricvalues[0] + barycentricvalues[1] + barycentricvalues[2]);
			standinguv = glm::vec2(0, 0);
			for (uint32_t x = 0; x < 3; x++) {
				standinguv += standingtri->vertices[x]->uv * barycentricvalues[x];
			}
		}

		if (camera->getPosition() != cameraPO.position) {
			camera->setPosition(cameraPO.position);
			GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] |= CAMERA_POSITION_CHANGE_FLAG_BIT;
		} else GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] &= ~CAMERA_POSITION_CHANGE_FLAG_BIT;

		//below evaluates to false when moving
//		GraphicsHandler::troubleshootingsstrm<<"POpos==coll? "<<(cameraPO.position==collision?"yeah":"no")
//		<<"\nPOposy==colly? "<<(cameraPO.position.y==triintersectiony?"yeah":"no")<<'\n';
//		GraphicsHandler::troubleshootingsstrm<<"collision: "<<SHIFT_TRIPLE(collision)<<'\n';
//		GraphicsHandler::troubleshootingsstrm<<"dt: "<<dt<<"\n";
//		GraphicsHandler::troubleshootingsstrm<<"camera PO position: "<<SHIFT_TRIPLE(cameraPO.position)<<'\n';
//		GraphicsHandler::troubleshootingsstrm<<"velocity: "<<SHIFT_TRIPLE(cameraPO.velocity)<<'\n';
//		GraphicsHandler::troubleshootingsstrm<<"acceleration: "<<SHIFT_TRIPLE(cameraPO.acceleration)<<'\n';
	}else updateStandingTri(land);
}

void PhysicsHandler::update (const std::vector<Tri>&land){
	dt = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - lasttime).count();
	lasttime = std::chrono::high_resolution_clock::now();
	//should the lasttime reset go here???
	updateCameraPos(land);
}

glm::vec3 PhysicsHandler::getWindVelocity (glm::vec3 p) {
	return glm::vec3(0.0, 0.0, 1.0);
}
