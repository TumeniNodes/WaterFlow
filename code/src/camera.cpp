/// @file fileHandler.cpp
/// @brief Implementations of functions FileHandler.h
// Written by Gustav Svensk, acquired from
// https://github.com/DrDante/TSBK03/
// with permission, 2015-09-24.

#include "camera.h"
#include "gtc/type_ptr.hpp"
#include "gtx/rotate_vector.hpp"
#include <cmath>
#include <iostream>

// ===== Constructor =====

Camera::Camera(glm::vec3 startPos, int* initScreenW, int* initScreenH, int tH, int tW, int xzL, int yLL, int yLH, DataHandler* terr)
{
	unlocked = false; 
	position = startPos;
	fi = 0.0f;
	theta = (GLfloat)M_PI / 2.0f;
	
    up = glm::vec3(0,1,0);

	screenW = initScreenW;
	screenH = initScreenH;

	terrH = tH;
	terrW = tW;
	xzLim = xzL;
	yLimLo = yLL;
	yLimHi = yLH;
	terrain = terr;

	rotSpeed = 0.01f;
	speed = 1.0f;
	drawDistance = 10000.0f;

	isFrozen = false;

	changeLookAtPos(0,0);
	updateVTP();
}

// ===== Methods =====

bool Camera::isInCollisionBox(glm::vec3 transVec, bool xz)
{
	if (unlocked) {
		return true;
	}
	glm::vec3 testVec = glm::vec3(glm::translate(glm::mat4(1.0f), transVec) * glm::vec4(position, 1));
	bool okToMove = true;
	if (xz)
	{
		// Check x.
		okToMove = okToMove && testVec.x > -xzLim && testVec.x < terrH + xzLim;
		// Check z.
		okToMove = okToMove && testVec.z > -xzLim && testVec.z < terrW + xzLim;
	}
	else
	{
		// Check y.
		okToMove = okToMove && testVec.y >= terrain->giveHeight(testVec.x, testVec.z) + yLimLo && testVec.y < terrain->getTerrainScale() + yLimHi;
	}
	return okToMove;
}

void Camera::unlock(){
	unlocked = true;
}

void Camera::rotate(char direction, GLfloat angle)
{
	switch (direction){
	case 'x':
		lookAtPos = glm::rotate(lookAtPos, angle, glm::vec3(1, 0, 0));
		break;
	case 'y':
		lookAtPos = glm::rotate(lookAtPos, angle, glm::vec3(0, 1, 0));
		break;
	case 'z':
		lookAtPos = glm::rotate(lookAtPos, angle, glm::vec3(0, 0, 1));
		break;
	}

	updateWTV();
}

void Camera::translate(GLfloat dx, GLfloat dy, GLfloat dz)
{
	if (!isFrozen) {
		glm::vec3 testVec = glm::vec3(dx, dy, dz);
		if (!isInCollisionBox(glm::vec3(0, dy, 0), false))
		{
			if (dy > 0)
			{
				testVec.y = 0;
			}
			else
			{
				testVec.y = terrain->giveHeight(position.x, position.z) + yLimLo - position.y;
			}
		}
		if (!isInCollisionBox(glm::vec3(dx, 0, 0), true))
		{
			testVec.x = 0;
		}
		if (!isInCollisionBox(glm::vec3(0, 0, dz), true))
		{
			testVec.z = 0;
		}
		position = glm::vec3(glm::translate(glm::mat4(1.0f), testVec) * glm::vec4(position, 1));
		lookAtPos = glm::vec3(glm::translate(glm::mat4(1.0f), testVec) * glm::vec4(lookAtPos, 1));
		updateWTV();
	}
}

void Camera::forward(GLfloat d)
{
	glm::vec3 forward_vec = lookAtPos - position;
	forward_vec = speed * d * forward_vec;
	translate(forward_vec.x, forward_vec.y, forward_vec.z);
}

void Camera::strafe(GLfloat d)
{
	glm::vec3 strafe_vec = lookAtPos - position;
	strafe_vec = glm::cross(up, strafe_vec);
	strafe_vec = glm::normalize(strafe_vec);
	strafe_vec = speed * d * strafe_vec;
	translate(strafe_vec.x, strafe_vec.y, strafe_vec.z);
}

void Camera::jump(GLfloat d) {
	glm::vec3 temp_vec = lookAtPos - position;
	glm::vec3 jump_vec = glm::cross(up, temp_vec);
	jump_vec = glm::cross(temp_vec, jump_vec);
	jump_vec = glm::normalize(jump_vec);
	jump_vec = speed * d * jump_vec;
	translate(jump_vec.x, jump_vec.y, jump_vec.z);
}

void Camera::updateWTV()
{
	WTVMatrix = glm::lookAt(position, lookAtPos, up);
}

void Camera::updateVTP() {
	float ratio = (GLfloat)(*screenW) / (GLfloat)(*screenH);
	VTPMatrix = glm::perspective((GLfloat)M_PI / 2, ratio, 1.0f, drawDistance);
}

void Camera::uploadCamData(GLuint program)
{
	glUseProgram(program);
	glUniformMatrix4fv(glGetUniformLocation(program, "VTPMatrix"), 1, GL_FALSE, glm::value_ptr(VTPMatrix));
	glUniformMatrix4fv(glGetUniformLocation(program, "WTVMatrix"), 1, GL_FALSE, glm::value_ptr(WTVMatrix));
	glUniform3fv(glGetUniformLocation(program, "camPos"), 1, glm::value_ptr(position));
}

void Camera::toggleFrozen() {
	isFrozen = !isFrozen;
}

void Camera::changeLookAtPos(int xrel, int yrel)
{
	if (!isFrozen) {
		float eps = 0.001f;

		fi += rotSpeed * xrel;
		theta += rotSpeed * yrel;
		
		fi = fmod(fi, 2.0f * (GLfloat)M_PI);
		theta = theta < (GLfloat)M_PI - eps ? (theta > eps ? theta : eps) : (GLfloat)M_PI - eps;

		lookAtPos.x = -sin(theta)*sin(fi) + position.x;
		lookAtPos.y = cos(theta) + position.y;
		lookAtPos.z = sin(theta)*cos(fi) + position.z;

		updateWTV();
	}
}

// ===== Getters =====

glm::mat4* Camera::getWTV() {
	return &WTVMatrix;
}
glm::mat4* Camera::getVTP() {
	return &VTPMatrix;
}
glm::vec3* Camera::getPos() {
	return &position;
}
GLfloat Camera::getSpeed() {
	return speed;
}
GLfloat* Camera::getSpeedPtr() {
	return &speed;
}
GLfloat Camera::getRotSpeed() {
	return rotSpeed;
}
GLfloat* Camera::getRotSpeedPtr() {
	return &rotSpeed;
}