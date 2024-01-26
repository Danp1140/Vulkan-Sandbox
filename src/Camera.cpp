//
// Created by Daniel Paavola on 5/30/21.
//

#include "Camera.h"

Camera::Camera () {
	position = glm::vec3(1, 1, 1);
	forward = glm::vec3(-1, -1, -1);
	up = glm::vec3(0, 1, 0);
	fovy = 0.785f;
	nearclip = 0.001f;
	farclip = 100.0f;
	aspectratio = 16.0f / 9.0f;
	movementsens = 0.1f;
	mousesens = 0.005f;
	projection = PERSPECTIVE;
	mousex = 0.0f;
	mousey = 0.0f;
	lastmousex = 0.0f;
	lastmousey = 0.0f;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
}

Camera::Camera (GLFWwindow* w, int hres, int vres) {
	position = glm::vec3(5, 5, 5);
	forward = glm::vec3(-1, -1, -1);
	up = glm::vec3(0, 1, 0);
	fovy = 0.785f;
	nearclip = 0.001f;
	farclip = 50.;
	aspectratio = float(hres) / float(vres);
	movementsens = 0.1f;
	mousesens = 0.005f;
	projection = PERSPECTIVE;
	glfwGetCursorPos(w, &mousex, &mousey);
	lastmousex = mousex;
	lastmousey = mousey;
	recalculateProjectionMatrix();
	recalculateViewMatrix();
}

void Camera::takeInputs (GLFWwindow* w) {
	lastmousex = mousex;
	lastmousey = mousey;
	glfwGetCursorPos(w, &mousex, &mousey);
	if (lastmousex != mousex || lastmousey != mousey) {
		GraphicsHandler::changeflags[GraphicsHandler::swapchainimageindex] |= CAMERA_LOOK_CHANGE_FLAG_BIT;
		double angletemp = -(mousex - lastmousex) / 2.0f * mousesens;
		glm::vec4 forwardtemp = glm::vec4(0, forward.x, forward.y, forward.z);
		forward = glm::rotate(glm::quat(cos(angletemp),
										up.x * sin(angletemp),
										up.y * sin(angletemp),
										up.z * sin(angletemp)),
							  forward);
		glm::vec3 axistemp = glm::cross(forward, up);
		angletemp = -(mousey - lastmousey) / 2.0f * mousesens;
		forward = glm::rotate(glm::quat(cos(angletemp), axistemp.x * sin(angletemp), axistemp.y * sin(angletemp),
										axistemp.z * sin(angletemp)), forward);
	}
	recalculateViewMatrix();
}

void Camera::calculateFrustum (glm::vec3* dst) {
	dst[0] = position + glm::vec3(0., 0.5, 0.);
	glm::vec3 vhat = glm::normalize(forward),
			rhat = glm::normalize(glm::cross(forward, glm::vec3(0., 1., 0))),
			uhat = glm::normalize(glm::cross(rhat, forward));
	float tanhalftheta = tan(aspectratio * fovy / 2), tanhalfphi = tan(fovy / 2.);
	dst[1] = position + glm::vec3(0., 0.5, 0.) + farclip * (vhat + tanhalftheta * rhat + tanhalfphi * uhat);
	dst[2] = position + glm::vec3(0., 0.5, 0.) + farclip * (vhat - tanhalftheta * rhat + tanhalfphi * uhat);
	dst[3] = position + glm::vec3(0., 0.5, 0.) + farclip * (vhat + tanhalftheta * rhat - tanhalfphi * uhat);
	dst[4] = position + glm::vec3(0., 0.5, 0.) + farclip * (vhat - tanhalftheta * rhat - tanhalfphi * uhat);
}

void Camera::recalculateProjectionMatrix () {
	if (projection == PERSPECTIVE) projectionmatrix = glm::perspective(fovy, aspectratio, nearclip, farclip);
	else projectionmatrix = glm::mat4(1);
	projectionmatrix[1][1] *= -1;
}

void Camera::recalculateViewMatrix () {
	viewmatrix = glm::lookAt(position + glm::vec3(0.0f, 0.5f, 0.0f),
							 position + forward + glm::vec3(0.0f, 0.5f, 0.0f),
							 up);
}

void Camera::setPosition (glm::vec3 p) {
	position = p;
	recalculateViewMatrix();
}

glm::mat4 Camera::calcAndGetSkyboxViewMatrix () {
	return glm::lookAt(glm::vec3(0., 0., 0.), forward, up);
}
