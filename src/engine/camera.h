#ifndef CAMERA_H
#define CAMERA_H

#include "vulkan/global.h"
#include <SDL3/SDL.h>
#include <cglm/cglm.h>

/****************Structs**************/
struct Camera {
	vec3 pos;
	vec3 front;
	vec3 up;

	bool firstMouse;

	float yaw;
	float pitch;
	float fov;
};

/****************Functions**************/
int setupCamera(struct Camera* camera)
{
	camera->pos[0] = 0.0f;
	camera->pos[1] = 0.0f;
	camera->pos[2] = 3.0f;
	camera->front[0] = 0.0f;
	camera->front[1] = 0.0f;
	camera->front[2] = -1.0f;
	camera->up[0] = 0.0f;
	camera->up[1] = 1.0f;
	camera->up[2] = 0.0f;

	camera->firstMouse = true;

	camera->yaw = -90.f;
	camera->pitch = 0.f;
	camera->fov = 45.f;

	return 0;
}

int processKeyboardCamera(struct Camera* cam, double deltaTime)
{
		const bool* keyboard = SDL_GetKeyboardState(NULL);

		float cameraSpeed = 0.01f * deltaTime;
		vec3 cross, dest;

		glm_vec3_scale(cam->front, cameraSpeed, dest);

		if(keyboard[SDL_SCANCODE_W])
			glm_vec3_add(cam->pos, dest, cam->pos);
		if(keyboard[SDL_SCANCODE_S])
			glm_vec3_sub(cam->pos, dest, cam->pos);

		glm_cross(cam->front, cam->up, cross);
		glm_normalize(cross);
		glm_vec3_scale(cross, cameraSpeed, dest);

		if(keyboard[SDL_SCANCODE_A])
			glm_vec3_sub(cam->pos, dest, cam->pos);
		if(keyboard[SDL_SCANCODE_D])
			glm_vec3_add(cam->pos, dest, cam->pos);

	return 0;
}

int processMouseCamera(struct Camera* cam)
{
	float xpos, ypos;

	SDL_GetRelativeMouseState(&xpos, &ypos);

	if(cam->firstMouse)
	{
		xpos = 0;
		ypos = 0;
		cam->firstMouse = false;
	}

	float sensitivity = 0.1f;
	xpos *= sensitivity;
	ypos *= sensitivity;

	cam->yaw += xpos;
	cam->pitch -= ypos;

	if (cam->pitch > 89.0f)
		cam->pitch = 89.0f;
	if (cam->pitch < -89.0f)
		cam->pitch = -89.0f;

	vec3 direction;
	direction[0] = cos(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
	direction[1] = sin(glm_rad(cam->pitch));
	direction[2] = sin(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
	glm_normalize_to(direction, cam->front);

	return 0;
}

int sendViewMat(struct Camera cam, UniformBufferObject* ubo)
{
	vec3 center;
	if(cam.pos[2] < 0)
		glm_vec3_sub(cam.pos, cam.front, center);
	else
		glm_vec3_add(cam.pos, cam.front, center);

	glm_lookat(cam.pos, center, cam.up, ubo->view);

	return 0;
}

#endif //CAMERA_H
