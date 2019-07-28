#include "regl-cpp.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdlib>

#include <cstring>
#include <string>
#include <ctime>

#include <vector>
#include <string>

#define _USE_MATH_DEFINES

#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

#include <chrono>
#include <thread>


GLFWwindow* window;

#define  LOGI(...)  printf(__VA_ARGS__)
#define  LOGE(...)  printf(__VA_ARGS__)


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

#define DEBUG_GROUPS

// these two are pretty useful, when debugging in RenderDoc or Nsight for instance.
void dpush(const char* str) {
#ifdef DEBUG_GROUPS
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, str);
#endif
}
void dpop() {
#ifdef DEBUG_GROUPS
	glad_glPopDebugGroup();
#endif
}

class vec2 {
public:
	float x, y;

	vec2(float x, float y) { this->x = x; this->y = y; }

	vec2() { this->x = this->y = 0; }
};

//
//
// Begin vec3.
//
//

float clamp(float v, float min, float max) { if (v < min) { return min; } else if (v > max) { return max; } else { return v; } }

class vec3 {
public:
	float x, y, z;

	vec3(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

	vec3() { this->x = this->y = this->z = 0; }

	vec3& operator+=(const vec3& b) { (*this) = (*this) + b; return (*this); }

	friend vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
	friend vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	friend vec3 operator*(const float s, const vec3& a) { return vec3(s * a.x, s * a.y, s * a.z); }
	friend vec3 operator*(const vec3& a, const float s) { return s * a; }

	static float length(const vec3& a) { return sqrt(vec3::dot(a, a)); }

	// dot product.
	static float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	static float distance(const vec3& a, const vec3& b) { return length(a - b); }
	static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }

	static vec3 mix(const vec3& x, const vec3& y, const float a) { return x * (1.0f - a) + y * a; }


	// cross product.
	static vec3 cross(const vec3& a, const vec3& b) { return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

	//
	// Rotate the vector 'v' around the 'axis' for 'theta' degrees.
	//
	static vec3 rotate(const vec3& v, const float theta, const vec3& axis) {
		vec3 k = vec3::normalize(axis); // normalize for good measure.
		return v * cos(theta) + vec3::cross(k, v)* sin(theta) + (k * vec3::dot(k, v)) * (1.0f - cos(theta));
	}

	static vec3 clamp(const vec3& v, const float min, const float max) {
		return vec3(
			::clamp(v.x, min, max), ::clamp(v.y, min, max), ::clamp(v.z, min, max)
		//0.0f, 0.0f, 0.0f
		);
	}

};

class vec4 {
public:
	float x, y, z, w;

	vec4(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }

	vec4() { this->x = this->y = this->z = 0; this->w = 0; }

	vec4& operator+=(const vec4& b) { (*this) = (*this) + b; return (*this); }

	friend vec4 operator-(const vec4& a, const vec4& b) { return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
	friend vec4 operator+(const vec4& a, const vec4& b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
	friend vec4 operator*(const float s, const vec4& a) { return vec4(s * a.x, s * a.y, s * a.z, s * a.w); }
	friend vec4 operator*(const vec4& a, const float s) { return s * a; }
	/*
	static float length(const vec3& a) { return sqrt(vec3::dot(a, a)); }

	// dot product.
	static float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	static float distance(const vec3& a, const vec3& b) { return length(a - b); }
	static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }
	*/
	//
	// Rotate the vector 'v' around the 'axis' for 'theta' degrees.
	// This is basically Rodrigues' rotation formula.
	//
};

//
//
// Begin mat4
//
//

class mat4 {
public:
	float m[4][4];

	mat4() {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m[i][j] = (i == j) ? 1.0f : 0.0f;
			}
		}
	}

	mat4 inverse() {

		float inv[16], det;

		float m00 = m[0][0];
		float m01 = m[1][0];
		float m02 = m[2][0];
		float m03 = m[3][0];
		
		float m10 = m[0][1];
		float m11 = m[1][1];
		float m12 = m[2][1];
		float m13 = m[3][1];

		float m20 = m[0][2];
		float m21 = m[1][2];
		float m22 = m[2][2];
		float m23 = m[3][2];

		float m30 = m[0][3];
		float m31 = m[1][3];
		float m32 = m[2][3];
		float m33 = m[3][3];


		inv[0] = m11 * m22 * m33 -
			m11 * m23 * m32 -
			m21 * m12  * m33 +
			m21 * m13  * m32 +
			m31 * m12  * m23 -
			m31 * m13  * m22;

		inv[4] = -m10 * m22 * m33 +
			m10 * m23 * m32 +
			m20 * m12  * m33 -
			m20 * m13  * m32 -
			m30 * m12  * m23 +
			m30 * m13  * m22;

		inv[8] = m10 * m21 * m33 -
			m10 * m23 * m31 -
			m20 * m11 * m33 +
			m20 * m13 * m31 +
			m30 * m11 * m23 -
			m30 * m13 * m21;

		inv[12] = -m10 * m21 * m32 +
			m10 * m22 * m31 +
			m20 * m11 * m32 -
			m20 * m12 * m31 -
			m30 * m11 * m22 +
			m30 * m12 * m21;

		inv[1] = -m01 * m22 * m33 +
			m01 * m23 * m32 +
			m21 * m02 * m33 -
			m21 * m03 * m32 -
			m31 * m02 * m23 +
			m31 * m03 * m22;

		inv[5] = m00 * m22 * m33 -
			m00 * m23 * m32 -
			m20 * m02 * m33 +
			m20 * m03 * m32 +
			m30 * m02 * m23 -
			m30 * m03 * m22;

		inv[9] = -m00 * m21 * m33 +
			m00 * m23 * m31 +
			m20 * m01 * m33 -
			m20 * m03 * m31 -
			m30 * m01 * m23 +
			m30 * m03 * m21;

		inv[13] = m00 * m21 * m32 -
			m00 * m22 * m31 -
			m20 * m01 * m32 +
			m20 * m02 * m31 +
			m30 * m01 * m22 -
			m30 * m02 * m21;

		inv[2] = m01 * m12 * m33 -
			m01 * m13 * m32 -
			m11 * m02 * m33 +
			m11 * m03 * m32 +
			m31 * m02 * m13 -
			m31 * m03 * m12;

		inv[6] = -m00 * m12 * m33 +
			m00 * m13 * m32 +
			m10 * m02 * m33 -
			m10 * m03 * m32 -
			m30 * m02 * m13 +
			m30 * m03 * m12;

		inv[10] = m00 * m11 * m33 -
			m00 * m13 * m31 -
			m10 * m01 * m33 +
			m10 * m03 * m31 +
			m30 * m01 * m13 -
			m30 * m03 * m11;

		inv[14] = -m00 * m11 * m32 +
			m00 * m12 * m31 +
			m10 * m01 * m32 -
			m10 * m02 * m31 -
			m30 * m01 * m12 +
			m30 * m02 * m11;

		inv[3] = -m01 * m12 * m23 +
			m01 * m13 * m22 +
			m11 * m02 * m23 -
			m11 * m03 * m22 -
			m21 * m02 * m13 +
			m21 * m03 * m12;

		inv[7] = m00 * m12 * m23 -
			m00 * m13 * m22 -
			m10 * m02 * m23 +
			m10 * m03 * m22 +
			m20 * m02 * m13 -
			m20 * m03 * m12;

		inv[11] = -m00 * m11 * m23 +
			m00 * m13 * m21 +
			m10 * m01 * m23 -
			m10 * m03 * m21 -
			m20 * m01 * m13 +
			m20 * m03 * m11;

		inv[15] = m00 * m11 * m22 -
			m00 * m12 * m21 -
			m10 * m01 * m22 +
			m10 * m02 * m21 +
			m20 * m01 * m12 -
			m20 * m02 * m11;

		det = m00 * inv[0] + m01 * inv[4] + m02 * inv[8] + m03 * inv[12];

		if (det == 0)
			LOGE("Determinant() = 0, so the matrix can not be inversed");

		det = 1.0f / det;

		mat4 res;

		

		res.m[0][0] = inv[0] * det;
		res.m[1][0] = inv[1] * det;
		res.m[2][0] = inv[2] * det;
		res.m[3][0] = inv[3] * det;

		res.m[0][1] = inv[4] * det;
		res.m[1][1] = inv[5] * det;
		res.m[2][1] = inv[6] * det;
		res.m[3][1] = inv[7] * det;

		res.m[0][2] = inv[8] * det;
		res.m[1][2] = inv[9] * det;
		res.m[2][2] = inv[10] * det;
		res.m[3][2] = inv[11] * det;

		res.m[0][3] = inv[12] * det;
		res.m[1][3] = inv[13] * det;
		res.m[2][3] = inv[14] * det;
		res.m[3][3] = inv[15] * det;
		
		return res;
	}

	// return perspective projection matrix.
	static mat4 perspective(float fovy, float aspect, float zNear, float zFar) {

		mat4 m;

		float ymax = zNear * tan(fovy);
		float xmax = ymax * aspect;

		float left = -xmax;
		float right = +xmax;

		float bottom = -ymax;
		float top = +ymax;

		float f1, f2, f3, f4;
		f1 = 2.0f * zNear;
		f2 = right - left;
		f3 = top - bottom;
		f4 = zFar - zNear;

		m.m[0][0] = f1 / f2;
		m.m[1][1] = f1 / f3;
		m.m[2][2] = (-zFar - zNear) / f4;

		m.m[2][0] = (right + left) / f2;
		m.m[2][1] = (top + bottom) / f3;

		m.m[2][3] = -1;
		m.m[3][2] = (-zFar * f1) / f4;

		m.m[3][3] = 0.0f;
		return m;
	}

	static mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up)
	{
		mat4 m;
		// compute the basis vectors.
		vec3 forward = vec3::normalize(center - eye); // forward vector.
		vec3 left = vec3::normalize(vec3::cross(forward, up)); // left vector.
		vec3 u = vec3::cross(left, forward); // up vector.

		m.m[0][0] = left.x;
		m.m[1][0] = left.y;
		m.m[2][0] = left.z;
		m.m[0][1] = u.x;
		m.m[1][1] = u.y;
		m.m[2][1] = u.z;
		m.m[0][2] = -forward.x;
		m.m[1][2] = -forward.y;
		m.m[2][2] = -forward.z;
		m.m[3][0] = -vec3::dot(left, eye);
		m.m[3][1] = -vec3::dot(u, eye);
		m.m[3][2] = vec3::dot(forward, eye);

		return m;
	}

	static mat4 orthographic(
		float left, float right,

		float bottom, float top,
		float znear, float zfar
	) {

		mat4 m;

		m.m[0][0] = 2.0f / (right - left);
		m.m[1][1] = 2.0f / (top - bottom);
		m.m[2][2] = 2.0f / (zfar - znear);

		m.m[3][0] = -(right + left) / (right - left);

		m.m[3][1] = -(top + bottom) / (top - bottom);
		m.m[3][2] = -(zfar + znear) / (zfar - znear);

		m.m[3][3] = 1.0f;

		return m;
	}

	friend mat4 operator*(const mat4& a, const mat4& b) {
		mat4 m;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m.m[i][j] = 0.0f;
				for (int k = 0; k < 4; k++) {
					m.m[i][j] += a.m[i][k] * b.m[k][j];
				}
			}
		}
		return m;
	}
	
	friend vec4 operator*(const vec4& v, const mat4& m) {
		float res[4];
		res[0] = 0.0f;
		res[1] = 0.0f;
		res[2] = 0.0f;
		res[3] = 0.0f;

		res[0] += m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;

		res[1] += m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;

		res[2] += m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;

		res[3] += m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

		return vec4(res[0], res[1], res[2], res[3]);
	}
};


//
//
// begin Camera
//
//
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

	void Update(const float delta) {

		// we use mouse movement to change the camera viewing angle.
		prevMouseX = curMouseX;
		prevMouseY = curMouseY;
		glfwGetCursorPos(window, &curMouseX, &curMouseY);
		float mouseDeltaX = (float)(curMouseX - prevMouseX);
		float mouseDeltaY = (float)(curMouseY - prevMouseY);


		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			viewDir = vec3::normalize(vec3::rotate(viewDir, mouseDeltaX*-0.01f, up));
			viewDir = vec3::normalize(vec3::rotate(viewDir, mouseDeltaY*-0.01f, right));
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
			LOGI("vec3(%f,%f,%f), vec3(%f,%f,%f),\n\n",
				position.x, position.y, position.z,
				viewDir.x, viewDir.y, viewDir.z
			);
		}
	}

	vec3 GetPosition() const {
		return position;
	}
};

float zNear = 0.1f;
float zFar = 8000.0f;
float fovy = 50.0f;

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

GLuint vao;

int FRAME_RATE = 30;
int fbWidth, fbHeight;

Camera camera(vec3(0, 0, 0), vec3(0, 0, 0));

void error_callback(int error, const char* description)
{
	puts(description);
}

void InitGlfw() {
	if (!glfwInit())
		exit(EXIT_FAILURE);

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

std::array<std::array<float, 4>, 4> toArr(const mat4& matrix) {
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

reglCpp::VertexBuffer cubePosBuffer;
reglCpp::VertexBuffer cubeNormalBuffer;
reglCpp::IndexBuffer cubeIndexBuffer;

void renderFrame() {

	using namespace reglCpp;

	mat4 viewMatrix = camera.GetViewMatrix();

	float ratio = (float)(WINDOW_WIDTH) / (float)WINDOW_HEIGHT;

	mat4 projectionMatrix = mat4::perspective(0.872665f * 0.5f, (float)(fbWidth) / (float)fbHeight, zNear, zFar);

	mat4 modelMatrix;

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


	Command drawCmd = Command()
		.clearColor({ 0.0f, 0.0f, 0.0f, 1.0f })
		.clearDepth(1.0f)

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
			{ "aNormal", &cubeNormalBuffer }})
		.indices(&cubeIndexBuffer)
		.count(3)
		.uniforms({
			{ "uModifier", { 1.0f, 1.0f, 0.2f } },
			{ "uModelMatrix", toArr(modelMatrix) },
			{ "uViewProjectionMatrix", toArr(viewProjectionMatrix) },
			});
	
	reglCpp::context.frame([&drawCmd]() {
		reglCpp::context.submit(drawCmd);
	});
}

//take the ratio of width/height into account for the calculation. may result in less banding artifacts.
//also, downscale first. I thin the downscaled may have less banding artifacts.

void HandleInput() {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	camera.Update(1.0f / (float)FRAME_RATE);
}

void setupGraphics() {
	InitGlfw();

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

	std::vector<unsigned int> indexData{ 0, 1, 2};
	
	cubeIndexBuffer =
		reglCpp::IndexBuffer()
		.data(indexData.data())
		.length((unsigned int)indexData.size() / 1)
		.name("cube index buffer")
		.finish();
}