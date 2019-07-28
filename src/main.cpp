#include "regl-cpp.hpp"

#include "glfw-util.hpp"

std::array<std::array<float, 4>, 4> toArr(const mat4 & matrix) {
	mat4 m = matrix;
	std::array<std::array<float, 4>, 4> ret{
	{
		{ m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3] },
		{ m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3] },
		{ m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3] },
		{ m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3] },
	}
	};

	return ret;
}

void demo() {
	reglCpp::VertexBuffer cubePosBuffer;
	reglCpp::VertexBuffer cubeNormalBuffer;
	reglCpp::IndexBuffer cubeIndexBuffer;

	camera = Camera(vec3(-0.277534f, 0.885269f, 2.221981f), vec3(-0.008268f, -0.841857f, -0.539637f));

	std::vector<float> posData{
		0.0900000036f, -0.0900000036f, 1.69000006f,
		0.0900000036f,  0.0900000036f, 1.69000006f,
		-0.0900000036f, 0.0900000036f, 1.69000006f
	};


	cubePosBuffer =
		reglCpp::VertexBuffer()
		.data(posData.data())
		.length((unsigned int)posData.size() / 3)
		.numComponents(3)
		.name("cube normal buffer")
		.finish();

	std::vector<float> normalData{
			0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f
	};

	cubeNormalBuffer =
		reglCpp::VertexBuffer()
		.data(normalData.data())
		.length((unsigned int)normalData.size() / 3)
		.numComponents(3)
		.name("cube normal buffer")
		.finish();

	std::vector<unsigned int> indexData{ 0, 1, 2 };

	cubeIndexBuffer =
		reglCpp::IndexBuffer()
		.data(indexData.data())
		.length((unsigned int)indexData.size() / 1)
		.name("cube index buffer")
		.finish();


	startRenderLoop([&]() {

		using namespace reglCpp;

		float zNear = 0.1f;
		float zFar = 8000.0f;
		float ratio = (float)(WINDOW_WIDTH) / (float)WINDOW_HEIGHT;
		mat4 projectionMatrix = mat4::perspective(0.872665f * 0.5f, (float)(WINDOW_WIDTH) / (float)WINDOW_HEIGHT, zNear, zFar);

		mat4 viewMatrix = camera.GetViewMatrix();

		
		mat4 viewProjectionMatrix = viewMatrix * projectionMatrix;

		std::array<std::array<float, 4>, 4> view{
			{
			{ 1.0f, 2.0f, 1.0f, 5.0f },
			{ 1.0f, 2.0f, 1.0f, 5.0f },
			{ 1.0f, 2.0f, 1.0f, 5.0f },
			{ 1.0f, 2.0f, 1.0f, 5.0f },
			}
		};
		std::vector<Uniform> lol;

		Command clearCmd = Command()
			.clearColor({ 0.2f, 0.2f, 0.2f, 1.0f })
			.clearDepth(1.0f)
			.viewport(0, 0, fbWidth, fbHeight);
		
		Command baseCmd = Command()
			.viewport(0, 0, fbWidth, fbHeight)
			.depthTest(true)
			.vert(R"V0G0N(  
in vec3 aPosition;
in vec3 aNormal;

out vec3 fsNormal;
out vec3 fsPos;

uniform mat4 uViewProjectionMatrix;
uniform mat4 uModelMatrix;

void main()
{
    fsNormal =  (uModelMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(aPosition, 1.0);
    fsPos = (uModelMatrix * vec4(aPosition, 1.0)).xyz;
}

				)V0G0N")
			.frag(R"V0G0N(  

in vec3 fsNormal;
in vec3 fsPos;

out vec4 fragData0;

uniform vec3 uModifier;

void main()
{
    fragData0 = vec4(fsNormal.xyz * uModifier, 1.0);

}

				)V0G0N")
			.attributes({
				{ "aPosition", &cubePosBuffer },
				{ "aNormal", &cubeNormalBuffer } })
				.indices(&cubeIndexBuffer)
			.count(3)
			.uniforms({
				{ "uViewProjectionMatrix", toArr(viewProjectionMatrix) },
				{ "uModifier",{1.0f, 1.0f, 0.3f} },

				});
		
		reglCpp::context.frame([&baseCmd, &clearCmd]() {
			
			reglCpp::context.submit(clearCmd);
			
			reglCpp::context.submit(baseCmd, []() {
			
				{
					mat4 modelMatrix;
					modelMatrix.m[3][0] = -0.5f;

					Command triCmd = Command()
						.uniforms({
							{ "uModelMatrix", toArr(modelMatrix) },
							});

					reglCpp::context.submit(triCmd);
				}

				{
					mat4 modelMatrix = mat4();
					modelMatrix.m[3][0] = +0.5f;

					Command triCmd = Command()
						.uniforms({
							{ "uModelMatrix", toArr(modelMatrix) },
							});

					reglCpp::context.submit(triCmd);
				}
			});
			
		
		});
	});
	

	cubeNormalBuffer.dispose();
	cubePosBuffer.dispose();
	cubeIndexBuffer.dispose();
	
	reglCpp::context.dispose();
}

int main(int argc, char** argv) {
	initGlfw(demo);
}