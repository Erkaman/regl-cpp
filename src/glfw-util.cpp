#include "glfw-util.hpp"

#include <chrono>
#include <thread>

#include <stdlib.h>

GLFWwindow* window;

GLuint vao;

int FRAME_RATE = 30;
int fbWidth;
int fbHeight;

Camera camera(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0));

void error_callback(int error, const char* description)
{
	puts(description);
}

inline void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("OpenGL error %08x, at %s:%i - for %s.\n", err, fname, line, stmt);
	}
}

#ifdef NDEBUG
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
    } while (0)
#else
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
	CheckOpenGLError(#stmt, __FILE__, __LINE__);	\
    } while (0)
#endif

void Camera::Update(const float delta) {

	// we use mouse movement to change the camera viewing angle.
	prevMouseX = curMouseX;
	prevMouseY = curMouseY;
	glfwGetCursorPos(window, &curMouseX, &curMouseY);
	float mouseDeltaX = (float)(curMouseX - prevMouseX);
	float mouseDeltaY = (float)(curMouseY - prevMouseY);


	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		viewDir = vec3::normalize(vec3::rotate(viewDir, mouseDeltaX * -0.01f, up));
		viewDir = vec3::normalize(vec3::rotate(viewDir, mouseDeltaY * -0.01f, right));
		right = vec3::normalize(vec3::cross(viewDir, vec3(0.0f, 0.0f, 1.0f)));
		up = vec3(0.0f, 0.0f, 1.0f);
	}

	static float cameraSpeed;

	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		cameraSpeed = 9.1f;
	else
		cameraSpeed = 3.2f;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		position += delta * cameraSpeed * viewDir;
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		position += -delta * cameraSpeed * viewDir;

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		position += -delta * cameraSpeed * right;
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		position += +delta * cameraSpeed * right;

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		position += +delta * cameraSpeed * up;
	else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		position += -delta * cameraSpeed * up;

	if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
		printf("vec3(%f,%f,%f), vec3(%f,%f,%f),\n\n",
			position.x, position.y, position.z,
			viewDir.x, viewDir.y, viewDir.z
		);
	}
}

void HandleInput() {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	camera.Update(1.0f / (float)FRAME_RATE);
}

void initGlfw(const std::function<void()>& fn) {
	{
		if (!glfwInit())
			exit(1);

		glfwSetErrorCallback(error_callback);

		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_SAMPLES, 0);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Deferred Shading Demo", NULL, NULL);
		if (!window) {
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		glfwMakeContextCurrent(window);


		glfwSetWindowPos(window, 0, 30);

		// load GLAD.
		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		// Bind and create VAO, otherwise, we can't do anything in OpenGL.
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	}

	fn();

	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void startRenderLoop(const std::function<void()>& fn) {

	float frameStartTime = 0;
	float frameEndTime = 0;
	frameStartTime = (float)glfwGetTime();
	
	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();
		HandleInput();
		
		fn();

		glfwSwapBuffers(window);

		// FPS regulation code. we will ensure that a framerate of 30FPS is maintained.
		// and for simplicity, we just assume that the computer is always able to maintain a framerate of at least 30FPS.
		{
			frameEndTime = (float)glfwGetTime();
			float frameDuration = frameEndTime - frameStartTime;
			const float sleepDuration = 1.0f / 30.0f - frameDuration;
			if (sleepDuration > 0.0f) {

				std::this_thread::sleep_for(std::chrono::milliseconds((int)(sleepDuration * 1000.0f)));
			}
			frameStartTime = (float)glfwGetTime();
		}
	}
	
}