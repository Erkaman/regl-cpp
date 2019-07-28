
#include "logic.hpp"


int main(int argc, char** argv) {

	setupGraphics();

	float a = atan(4.9 / 40.0);
	
	float frameStartTime = 0;
	float frameEndTime = 0;
	frameStartTime = (float)glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		
		glfwPollEvents();
		HandleInput();
		renderFrame();
		glfwSwapBuffers(window);

		// FPS regulation code. we will ensure that a framerate of 30FPS is maintained.
		// and for simplicity, we just assume that the computer is always able to maintain a framerate of at least 30FPS.
		{
			frameEndTime = (float)glfwGetTime();
			float frameDuration = frameEndTime - frameStartTime;
			const float sleepDuration = 1.0f / 30.0f - frameDuration;
			if (sleepDuration > 0.0f) {
				
				std::this_thread::sleep_for(std::chrono::milliseconds((int)(sleepDuration  * 1000.0f)));
			}
			frameStartTime = (float)glfwGetTime();
		}
	}
	
	glfwTerminate();
	exit(EXIT_SUCCESS);
}