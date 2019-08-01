#pragma once

#include <math.h>
#include <stdio.h>
#include <functional>

#include "math.hpp"

// these two are pretty useful, when debugging in RenderDoc or Nsight for instance.
#define DEBUG_GROUPS // remove this to make the two below into no-ops.
void dpush(const char* str);
void dpop();

// camera that uses glfw for mouse and keyboard input.
class Camera {
public:

	vec3 viewDir;
	vec3 right;
	vec3 up;
	vec3 position;

	double prevMouseX = 0.0;
	double prevMouseY = 0.0;

	double curMouseX = 0.0;
	double curMouseY = 0.0;

public:
	mat4 GetViewMatrix() {
		return mat4::lookAt(position, position + viewDir, up);

	}

	Camera(const vec3& position_, const vec3& viewDir_) : position(position_), viewDir(viewDir_) {
		viewDir = vec3::normalize(viewDir);
		right = vec3::normalize(vec3::cross(viewDir, vec3(0.0f, 0.0f, 1.0f)));
		up = vec3(0, 0.0f, 1);
	}

	void Update(const float delta);

	vec3 GetPosition() const {
		return position;
	}
};

void setWindowSize(int width, int height);

int getFramebufferWidth();
int getFramebufferHeight();

extern Camera camera;

void initGlfw(const std::function<void()>& fn);
void startRenderLoop(const std::function<void()>& fn);
