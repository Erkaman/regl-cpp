#include "regl-cpp.hpp"

#include "glfw-util.hpp"


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

//using namespace tinygltf;

constexpr int BUFFER_VIEW_TARGET_ARRAY_BUFFER = 34962;
constexpr int BUFFER_VIEW_TARGET_ELEMENT_ARRAY_BUFFER = 34963;

constexpr int COMPONENT_TYPE_FLOAT = 5126;

constexpr int TYPE_VEC3 = 3;

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

void loadMesh(reglCpp::VertexBuffer& meshPosBuffer, int* numPoints) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn,
		"C:/Users/arneb/Documents/regl-cpp/samples/animated/CesiumMan.gltf");
	//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		exit(1);
		//	return -1;
	}

	if (model.meshes.size() != 1) {
		printf("We only handle file with a single mesh\n");
		exit(1);
	}

	tinygltf::Mesh gltfMesh = model.meshes[0];


	if (gltfMesh.primitives.size() != 1) {
		printf("We only handle file with a single primitive\n");
		exit(1);
	}
	tinygltf::Primitive gltfPrimtive = gltfMesh.primitives[0];

	if (gltfPrimtive.attributes.count("POSITION") == 0) {
		printf("Primtivie lacks a position attribute\n");
		exit(1);
	}

	tinygltf::Accessor gltfPositionAccesor = model.accessors[gltfPrimtive.attributes["POSITION"]];
	assert(gltfPositionAccesor.normalized == false);
	assert(gltfPositionAccesor.componentType == COMPONENT_TYPE_FLOAT);
	assert(gltfPositionAccesor.type == TYPE_VEC3);

	tinygltf::BufferView gltfPositionBufferView = model.bufferViews[gltfPositionAccesor.bufferView];
	assert(gltfPositionBufferView.target == BUFFER_VIEW_TARGET_ARRAY_BUFFER);
	assert(gltfPositionBufferView.byteStride == 12);

	const tinygltf::Buffer& gltfBuffer = model.buffers[gltfPositionBufferView.buffer];

	//int positionAttribute = gltfPrimtive.attributes["POSITION"];

	int bufferBeg = gltfPositionBufferView.byteOffset;
	int bufferEnd = bufferBeg + gltfPositionBufferView.byteLength;

	int iBuf = bufferBeg;

	iBuf += gltfPositionAccesor.byteOffset;

	const float xmin = gltfPositionAccesor.minValues[0];
	const float ymin = gltfPositionAccesor.minValues[1];
	const float zmin = gltfPositionAccesor.minValues[2];

	const float xmax = gltfPositionAccesor.maxValues[0];
	const float ymax = gltfPositionAccesor.maxValues[1];
	const float zmax = gltfPositionAccesor.maxValues[2];

	auto bytesToFloat = [](
		unsigned char b0,
		unsigned char b1,
		unsigned char b2,
		unsigned char b3) {
			unsigned char xBuf[] = { b0, b1, b2, b3 };
			float x;
			memcpy(&x, xBuf, sizeof(float) * 4);
			return x;
	};

	std::vector<float> posData;

	for (int iElem = 0; iElem < gltfPositionAccesor.count; ++iElem) {
		float x = bytesToFloat(
			gltfBuffer.data[iBuf + 0],
			gltfBuffer.data[iBuf + 1],
			gltfBuffer.data[iBuf + 2],
			gltfBuffer.data[iBuf + 3]);
		iBuf += 4;

		float y = bytesToFloat(
			gltfBuffer.data[iBuf + 0],
			gltfBuffer.data[iBuf + 1],
			gltfBuffer.data[iBuf + 2],
			gltfBuffer.data[iBuf + 3]);
		iBuf += 4;

		float z = bytesToFloat(
			gltfBuffer.data[iBuf + 0],
			gltfBuffer.data[iBuf + 1],
			gltfBuffer.data[iBuf + 2],
			gltfBuffer.data[iBuf + 3]);
		iBuf += 4;

		assert(x >= xmin && x <= xmax);
		assert(y >= ymin && y <= ymax);
		assert(z >= zmin && z <= zmax);

		posData.push_back(x);
		posData.push_back(y);
		posData.push_back(z);
	}
	
	meshPosBuffer =
		reglCpp::VertexBuffer()
		.data(posData.data())
		.length((unsigned int)posData.size() / 3)
		.numComponents(3)
		.name("mesh position buffer")
		.finish();

	*numPoints = posData.size() / 3;
}

void demo() {
	reglCpp::VertexBuffer meshPosBuffer;
	int numPoints;
	loadMesh(meshPosBuffer, &numPoints);

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
		float ratio = (float)(getFramebufferWidth()) / (float)getFramebufferHeight();
		mat4 projectionMatrix = mat4::perspective(0.872665f * 0.5f, (float)(getFramebufferWidth()) / (float)getFramebufferHeight(), zNear, zFar);

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
			.viewport(0, 0, getFramebufferWidth(), getFramebufferHeight());
		
		Command baseCmd = Command()
			.viewport(0, 0, getFramebufferWidth(), getFramebufferHeight())
			.depthTest(true)
			.vert(R"V0G0N(  
precision highp float;

attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec2 aUv;

varying vec2 fsUv;

uniform mat4 uViewProjectionMatrix;
uniform mat4 uModelMatrix;

void main()
{
	fsUv = aUv;
    gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(aPosition, 1.0);
}

				)V0G0N")
			.uniforms({
				{ "uViewProjectionMatrix", mat4::toArr(viewProjectionMatrix) },
				});
		
		reglCpp::context.frame([&]() {
			
			reglCpp::context.submit(clearCmd);
			
			reglCpp::context.submit(baseCmd, [&]() {
			
				{
					mat4 modelMatrix;
					modelMatrix.m[3][0] = -0.5f;

					Command triCmd = Command()
						
						.frag(R"V0G0N( 
precision highp float;
 
varying vec2 fsUv;
uniform sampler2D uTex;

void main()
{
	vec3 c = texture2D(uTex, fsUv.xy).rgb;
    gl_FragColor = vec4(c.xyz, 1.0);
}
				)V0G0N")
						.attributes({
							{ "aPosition", &cubePosBuffer },
							{ "aUv", &cubeUvBuffer },
							{ "aNormal", &cubeNormalBuffer } })
						.indices(&cubeIndexBuffer)
						.count((int)indexData.size())
						.primitive("triangles")
						.uniforms({
							{ "uModelMatrix", mat4::toArr(modelMatrix) },
							{ "uTex", &texture },
							});

					reglCpp::context.submit(triCmd);
				}

				{
					mat4 modelMatrix;
					//modelMatrix.m[3][0] = -0.5f;

					Command pointsCmd = Command()

						.frag(R"V0G0N( 
precision highp float;
 
varying vec2 fsUv;

void main()
{
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
				)V0G0N")
						.attributes({
							{ "aPosition", &meshPosBuffer } } )

						.count((int)numPoints)
						.primitive("points")
						.uniforms({
							{ "uModelMatrix", mat4::toArr(modelMatrix) },
							});

					reglCpp::context.submit(pointsCmd);
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
	setWindowSize((1920 * 3) / 4, (1080 * 3) / 4);
	initGlfw(demo);
}