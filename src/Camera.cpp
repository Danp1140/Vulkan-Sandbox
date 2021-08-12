//
// Created by Daniel Paavola on 5/30/21.
//

#include "Camera.h"

Camera::Camera(){
	position=glm::vec3(1, 1, 1);
	forward=glm::vec3(-1, -1, -1);
	up=glm::vec3(0, 1, 0);
	fovy=0.785f;
	nearclip=0.001f;
	farclip=100.0f;
	//THIS IS TEMPORARILY PRE-DEFINED, should find a way to set real value later
	aspectratio=16.0f/9.0f;
	movementsens=0.1f;
	mousesens=0.005f;
	projection=PERSPECTIVE;
	mousex=0.0f; mousey=0.0f;
	lastmousex=0.0f; lastmousey=0.0f;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
}

Camera::Camera(GLFWwindow*w, int hres, int vres){
	position=glm::vec3(0, 5, 0);
	forward=glm::vec3(-1, -1 , -1);
	up=glm::vec3(0, 1, 0);
	fovy=0.785f;
	nearclip=0.001f;
	farclip=100.0f;
	aspectratio=float(hres)/float(vres);
	movementsens=0.1f;
	mousesens=0.005f;
	projection=PERSPECTIVE;
	glfwGetCursorPos(w, &mousex, &mousey);
	lastmousex=mousex; lastmousey=mousey;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
}

void Camera::takeInputs(GLFWwindow*w){
	//may place this all in callback(s) if that is more efficient
	lastmousex=mousex; lastmousey=mousey;
	glfwGetCursorPos(w, &mousex, &mousey);
	if(lastmousex!=mousex||lastmousey!=mousey){
		GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex]|=CAMERA_LOOK_CHANGE_FLAG_BIT;
		double angletemp=-(mousex-lastmousex)/2.0f*mousesens;
		glm::vec4 forwardtemp=glm::vec4(0, forward.x, forward.y, forward.z);
		forward=glm::rotate(glm::quat(cos(angletemp), up.x*sin(angletemp), up.y*sin(angletemp), up.z*sin(angletemp)),
		                    forward);
		glm::vec3 axistemp=glm::cross(forward, up);
		angletemp=-(mousey-lastmousey)/2.0f*mousesens;
		forward=glm::rotate(glm::quat(cos(angletemp), axistemp.x*sin(angletemp), axistemp.y*sin(angletemp),
		                              axistemp.z*sin(angletemp)), forward);
	}
	recalculateViewMatrix();
}

void Camera::recalculateProjectionMatrix(){
	if(projection==PERSPECTIVE) projectionmatrix=glm::perspective(fovy, aspectratio, nearclip, farclip);
	else projectionmatrix=glm::mat4(1);
	projectionmatrix[1][1]*=-1;
}

void Camera::recalculateViewMatrix(){
	//height addition should be cleaned up later
	viewmatrix=glm::lookAt(position+glm::vec3(0.0f, 0.5f, 0.0f), position+forward+glm::vec3(0.0f, 0.5f, 0.0f), up);
}

void Camera::setPosition(glm::vec3 p){
	position=p;
	recalculateViewMatrix();
}