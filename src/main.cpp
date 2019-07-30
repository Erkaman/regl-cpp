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


void cubeData(
	std::vector<float>& posData,
	std::vector<float>& uvData,
	std::vector<float>& normalData,
	std::vector<unsigned int>& indexData) {

	posData = std::vector<float>{
		-0.5, +0.5, +0.5,  +0.5, +0.5, +0.5,  +0.5, -0.5, +0.5,  -0.5, -0.5, +0.5, // positive z face.
		+0.5, +0.5, +0.5,  +0.5, +0.5, -0.5,  +0.5, -0.5, -0.5,  +0.5, -0.5, +0.5, // positive x face
		+0.5, +0.5, -0.5,  -0.5, +0.5, -0.5,  -0.5, -0.5, -0.5,  +0.5, -0.5, -0.5, // negative z face
		-0.5, +0.5, -0.5,  -0.5, +0.5, +0.5,  -0.5, -0.5, +0.5,  -0.5, -0.5, -0.5, // negative x face.
		-0.5, +0.5, -0.5,  +0.5, +0.5, -0.5,  +0.5, +0.5, +0.5,  -0.5, +0.5, +0.5, // top face
		-0.5, -0.5, -0.5,  +0.5, -0.5, -0.5,  +0.5, -0.5, +0.5,  -0.5, -0.5, +0.5  // bottom face
	};


	normalData = std::vector<float>{
		// side faces
		0.0, 0.0, +1.0,  0.0, 0.0, +1.0,  0.0, 0.0, +1.0,  0.0, 0.0, +1.0,
		+1.0, 0.0, 0.0,  +1.0, 0.0, 0.0,  +1.0, 0.0, 0.0,  +1.0, 0.0, 0.0,
		0.0, 0.0, -1.0,  0.0, 0.0, -1.0,  0.0, 0.0, -1.0,  0.0, 0.0, -1.0,
		-1.0, 0.0, 0.0,  -1.0, 0.0, 0.0,  -1.0, 0.0, 0.0,  -1.0, 0.0, 0.0,
		// top
		0.0, +1.0, 0.0,  0.0, +1.0, 0.0,  0.0, +1.0, 0.0,  0.0, +1.0, 0.0,
		// bottom
		0.0, -1.0, 0.0,  0.0, -1.0, 0.0,  0.0, -1.0, 0.0,  0.0, -1.0, 0.0
	};

	uvData = std::vector<float>{
			0.0, 0.0, 4.0, 0.0, 4.0, 4.0, 0.0, 4.0, // positive z face.
			0.0, 0.0, 4.0, 0.0, 4.0, 4.0, 0.0, 4.0, // positive x face.
			0.0, 0.0, 4.0, 0.0, 4.0, 4.0, 0.0, 4.0, // negative z face.
			0.0, 0.0, 4.0, 0.0, 4.0, 4.0, 0.0, 4.0, // negative x face.
			0.0, 0.0, 4.0, 0.0, 4.0, 4.0, 0.0, 4.0, // top face
			0.0, 0.0, 4.0, 0.0, 4.0, 4.0, 0.0, 4.0  // bottom face
	};

	indexData = std::vector<unsigned int> {
		2, 1, 0, 2, 0, 3,       // positive z face.
		6, 5, 4, 6, 4, 7,       // positive x face.

		10, 9, 8, 10, 8, 11,    // negative z face.
		14, 13, 12, 14, 12, 15, // negative x face.
		18, 17, 16, 18, 16, 19, // top face.
		20, 21, 22, 23, 20, 22  // bottom face
	};

}


void demo() {
	reglCpp::VertexBuffer cubePosBuffer;
	reglCpp::VertexBuffer cubeNormalBuffer;
	reglCpp::VertexBuffer cubeUvBuffer;
	reglCpp::IndexBuffer cubeIndexBuffer;

	camera = Camera(vec3(-0.277534f, 0.885269f, 2.221981f), vec3(-0.008268f, -0.841857f, -0.539637f));

	std::vector<float> posData;
	std::vector<float> normalData;
	std::vector<float> uvData;
	std::vector<unsigned int> indexData;

	cubeData(
		posData,
		uvData,
		normalData,
		indexData);

	cubePosBuffer =
		reglCpp::VertexBuffer()
		.data(posData.data())
		.length((unsigned int)posData.size() / 3)
		.numComponents(3)
		.name("cube normal buffer")
		.finish();

	cubeNormalBuffer =
		reglCpp::VertexBuffer()
		.data(normalData.data())
		.length((unsigned int)normalData.size() / 3)
		.numComponents(3)
		.name("cube normal buffer")
		.finish();
	
	cubeUvBuffer =
		reglCpp::VertexBuffer()
		.data(uvData.data())
		.length((unsigned int)uvData.size() / 2)
		.numComponents(2)
		.name("cube uv buffer")
		.finish();

	cubeIndexBuffer =
		reglCpp::IndexBuffer()
		.data(indexData.data())
		.length((unsigned int)indexData.size() / 1)
		.name("cube index buffer")
		.finish();

	reglCpp::Texture2D texture;



	
	std::vector<unsigned char> textureData = {
		255, 255, 255, 255,   0, 0, 255, 0 ,

		0, 255, 0, 0,   0, 0, 0, 255,
	};
	

	texture = 
		reglCpp::Texture2D()
		.data(textureData.data())
		.width(2)
		.height(2)
		.pixelFormat("rgba8")
		.min("linear mipmap linear")
		.mag("nearest")
		.wrap("repeat")
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
in vec2 aUv;

out vec3 fsNormal;
out vec3 fsPos;
out vec2 fsUv;

uniform mat4 uViewProjectionMatrix;
uniform mat4 uModelMatrix;

void main()
{
    fsNormal =  (uModelMatrix * vec4(aNormal, 0.0)).xyz;
	fsUv = aUv;
    gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(aPosition, 1.0);
    fsPos = (uModelMatrix * vec4(aPosition, 1.0)).xyz;
}

				)V0G0N")
			.frag(R"V0G0N(  

in vec3 fsNormal;
in vec3 fsPos;
in vec2 fsUv;

out vec4 fragData0;

uniform sampler2D uTex;

void main()
{
	vec3 c = texture(uTex, fsUv.xy).rgb;
    fragData0 = vec4(c.xyz, 1.0);
}

				)V0G0N")
			.uniforms({
				{ "uViewProjectionMatrix", toArr(viewProjectionMatrix) },
				});
		
		reglCpp::context.frame([&]() {
			
			reglCpp::context.submit(clearCmd);
			
			reglCpp::context.submit(baseCmd, [&]() {
			
				{
					mat4 modelMatrix;
					modelMatrix.m[3][0] = -0.5f;

					Command triCmd = Command()
						.attributes({
							{ "aPosition", &cubePosBuffer },
							{ "aUv", &cubeUvBuffer },

							{ "aNormal", &cubeNormalBuffer } })
						.indices(&cubeIndexBuffer)
						.count((int)indexData.size())
						.uniforms({
							{ "uModelMatrix", toArr(modelMatrix) },
							{ "uTex", &texture },
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