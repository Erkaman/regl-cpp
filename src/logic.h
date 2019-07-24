
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

#include <glad/glad.h>

#include <GLFW/glfw3.h>

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

enum GpuTimeStamp
{
	GTS_Shadow_Render,
	GTS_Gbuffer_output,
	GTS_Shading,
	GTS_Scattering,
	GTS_TAA,
	GTS_Blit,
	GTS_Max
};


float Time() {
	clock_t startcputime = std::clock();
	//  LOG_I("startcputime: %ld", startcputime);
	float cpu_duration = (float)(startcputime) / (float)CLOCKS_PER_SEC;
	return cpu_duration;
}

inline char* GetShaderLogInfo(GLuint shader) {
	GLint len;
	GLsizei actualLen;
	GL_C(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
	char* infoLog = new char[len];
	GL_C(glGetShaderInfoLog(shader, len, &actualLen, infoLog));
	return infoLog;
}

inline GLuint CreateShaderFromString(const std::string& shaderSource, const GLenum shaderType) {
	GLuint shader;

	GL_C(shader = glCreateShader(shaderType));
	const char *c_str = shaderSource.c_str();
	GL_C(glShaderSource(shader, 1, &c_str, NULL));
	GL_C(glCompileShader(shader));

	GLint compileStatus;
	GL_C(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus));
	if (compileStatus != GL_TRUE) {
		LOGI("Could not compile shader\n\n%s \n\n%s\n", shaderSource.c_str(),
			GetShaderLogInfo(shader));
		exit(1);
	}

	return shader;
}

inline GLuint LoadNormalShader(const std::string& vsSource, const std::string& fsShader) {

	bool useGl3 = true;// just hardcode this for now.

	std::string prefix = ""; 

	if (useGl3) {
		prefix += "#version 150\n";
		prefix += "#define GL3\n";
	}
	else {
		prefix += "#version 110\n";
		prefix += "#undef GL3\n";
	}
	
	prefix += std::string(R"(
#ifdef GL3

#define DECLARE_GBUFFER_OUTPUT out vec4 fragData0; out vec4 fragData1; out vec4 fragData2;
#define DECLARE_FRAG_COLOR out vec4 fragColor;

#define VS_IN_ATTRIB in
#define VS_OUT_ATTRIB out
#define FS_IN_ATTRIB in

#define MGL_FRAG_COLOR fragColor

#define MGL_FRAG_DATA0 fragData0
#define MGL_FRAG_DATA1 fragData1
#define MGL_FRAG_DATA2 fragData2

#define SAMPLE_TEX3D texture
#define SAMPLE_TEX2D texture

#else

#define DECLARE_GBUFFER_OUTPUT
#define DECLARE_FRAG_COLOR 

#define VS_IN_ATTRIB attribute
#define VS_OUT_ATTRIB varying
#define FS_IN_ATTRIB varying

#define MGL_FRAG_COLOR gl_FragColor

#define MGL_FRAG_DATA0 gl_FragData[0]
#define MGL_FRAG_DATA1 gl_FragData[1]
#define MGL_FRAG_DATA2 gl_FragData[2]

#define SAMPLE_TEX3D texture3D
#define SAMPLE_TEX2D texture2D

#endif
)");

	GLuint vs = CreateShaderFromString(prefix + vsSource, GL_VERTEX_SHADER);
	GLuint fs = CreateShaderFromString(prefix + fsShader, GL_FRAGMENT_SHADER);

	GLuint shader = glCreateProgram();
	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	GLint Result;
	glGetProgramiv(shader, GL_LINK_STATUS, &Result);
	if (Result == GL_FALSE) {
		LOGI("Could not link shader \n\n%s\n", GetShaderLogInfo(shader));
		exit(1);
	}

	glDetachShader(shader, vs);
	glDetachShader(shader, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader;
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

struct GeoVertex {
	float x, y, z; // position
	float nx, ny, nz; // normal
};

struct FullscreenVertex {
	float x, y, z, w;
};

struct Tri {
	GLuint indices[3];
};

struct Mesh {
	GLuint vertexVbo;
	GLuint indexVbo;
	GLuint indexCount;

	void Create(const std::vector<GeoVertex>& vertices, const std::vector<Tri>& indices) {
		// upload geometry to GPU.
		GL_C(glGenBuffers(1, &vertexVbo));
		GL_C(glBindBuffer(GL_ARRAY_BUFFER, vertexVbo));
		GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(GeoVertex)*vertices.size(), (float*)vertices.data(), GL_STATIC_DRAW));

		GL_C(glGenBuffers(1, &indexVbo));
		GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVbo));
		GL_C(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Tri)*indices.size(), indices.data(), GL_STATIC_DRAW));

		indexCount = (GLuint)indices.size() * 3;
	}
};

Mesh boxesMesh;
Mesh groundMesh;

GLuint geoShader;
GLint gsColorLocation;
GLint gsHistVpLocation;
GLuint gsViewProjectionMatrixLocation;
GLuint gsModelMatrixLocation;
GLint gsUnjitteredVpLocation;
GLint gsPositionAttribLocation;
GLint gsNormalAttribLocation;

GLuint frameFbo;

Camera camera(
	vec3(3, 2, 0),
	vec3::normalize(vec3(-1.0f, -0.3f, 0.0)));

void AddBox(
	float ox, float oy, float oz,
	float sx, float sy, float sz,
	std::vector<GeoVertex>& vertices, std::vector<Tri>& indices) {

	GLuint ibeg = (GLuint)vertices.size();

	vertices.push_back(GeoVertex{ -0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f });
	vertices.push_back(GeoVertex{ +0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f });
	vertices.push_back(GeoVertex{ +0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f });
	vertices.push_back(GeoVertex{ -0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f });

	vertices.push_back(GeoVertex{ +0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, +0.5f, -0.5f, +1.0f, +0.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, -0.5f, +0.5f, +1.0f, +0.0f, +0.0f });

	vertices.push_back(GeoVertex{ +0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f });
	vertices.push_back(GeoVertex{ -0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f });
	vertices.push_back(GeoVertex{ -0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f });
	vertices.push_back(GeoVertex{ +0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f });

	vertices.push_back(GeoVertex{ -0.5f, +0.5f, -0.5f, -1.0f, +0.0f, +0.0f });
	vertices.push_back(GeoVertex{ -0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f });
	vertices.push_back(GeoVertex{ -0.5f, -0.5f, +0.5f, -1.0f, +0.0f, +0.0f });
	vertices.push_back(GeoVertex{ -0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f });

	vertices.push_back(GeoVertex{ -0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f });
	vertices.push_back(GeoVertex{ -0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f });

	vertices.push_back(GeoVertex{ -0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f });
	vertices.push_back(GeoVertex{ +0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f });
	vertices.push_back(GeoVertex{ -0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f });

	int iend = (GLuint)vertices.size();

	for (int i = ibeg; i < iend; ++i) {
		GeoVertex& v = vertices[i];

		v.x = (v.x + 0.5f) * 2.0f * sx + (ox - sx);
		v.y = (v.y + 0.5f) * 2.0f * sy + (oy - sy);
		v.z = (v.z + 0.5f) * 2.0f * sz + (oz - sz);
	}

	indices.push_back(Tri{ 2 + ibeg,  1 + ibeg,  0 + ibeg });
	indices.push_back(Tri{ 2 + ibeg,  0 + ibeg,  3 + ibeg });
	indices.push_back(Tri{ 6 + ibeg,  5 + ibeg,  4 + ibeg });
	indices.push_back(Tri{ 6 + ibeg,  4 + ibeg,  7 + ibeg });
	indices.push_back(Tri{ 10 + ibeg,  9 + ibeg,  8 + ibeg });
	indices.push_back(Tri{ 10 + ibeg,  8 + ibeg, 11 + ibeg });
	indices.push_back(Tri{ 14 + ibeg, 13 + ibeg, 12 + ibeg });
	indices.push_back(Tri{ 14 + ibeg, 12 + ibeg, 15 + ibeg });
	indices.push_back(Tri{ 18 + ibeg, 17 + ibeg, 16 + ibeg });
	indices.push_back(Tri{ 18 + ibeg, 16 + ibeg, 19 + ibeg });
	indices.push_back(Tri{ 20 + ibeg, 21 + ibeg, 22 + ibeg });
	indices.push_back(Tri{ 23 + ibeg, 20 + ibeg, 22 + ibeg });
}

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

void renderShadowCasters(bool renderDepth) {

	{
		GL_C(glBindBuffer(GL_ARRAY_BUFFER, boxesMesh.vertexVbo));

		if (renderDepth) {
		}
		else {
			GL_C(glEnableVertexAttribArray((GLuint)gsPositionAttribLocation));
			GL_C(glVertexAttribPointer((GLuint)gsPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(GeoVertex), (void*)0));

			GL_C(glEnableVertexAttribArray((GLuint)gsNormalAttribLocation));
			GL_C(glVertexAttribPointer((GLuint)gsNormalAttribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(GeoVertex), (void*)(sizeof(float) * 3)));
		}
		
		GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxesMesh.indexVbo));

		if(!renderDepth) glUniform3f(gsColorLocation, 0.4f, 0.0f, 0.0f);
		glDrawElements(GL_TRIANGLES, boxesMesh.indexCount, GL_UNSIGNED_INT, 0);
	}


	// render ground.
	{
		GL_C(glBindBuffer(GL_ARRAY_BUFFER, groundMesh.vertexVbo));

		if (renderDepth) {

		}
		else {
			GL_C(glEnableVertexAttribArray((GLuint)gsPositionAttribLocation));
			GL_C(glVertexAttribPointer((GLuint)gsPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(GeoVertex), (void*)0));

			GL_C(glEnableVertexAttribArray((GLuint)gsNormalAttribLocation));
			GL_C(glVertexAttribPointer((GLuint)gsNormalAttribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(GeoVertex), (void*)(sizeof(float) * 3)));
		}
		GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundMesh.indexVbo));

		if (!renderDepth) glUniform3f(gsColorLocation, 0.1f, 0.1f, 0.1f);
		glDrawElements(GL_TRIANGLES, groundMesh.indexCount, GL_UNSIGNED_INT, 0);
	}

}

void checkFbo() {
	// make sure nothing went wrong:
	GLenum status;
	GL_C(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("Framebuffer not complete. Status: 0x%08x\n", status);
	}
}

//mat4 unjitteredProjectionMatrix;
mat4 histVp;
int iHalton = 0; // halton sample of the current frame.
mat4 unjitteredVp;

GLuint  pingPongHdrTex[2];
int iHistHdrTex = 1;

GLuint hdrTexture;

void renderFrame() {

	static float c = 0.0f;
	c += 0.11f;


	// setup matrices.

	// setup default GL state.
	GL_C(glEnable(GL_DEPTH_TEST));
	GL_C(glDepthMask(true));
	GL_C(glDisable(GL_BLEND));
	GL_C(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
	GL_C(glEnable(GL_CULL_FACE));
	GL_C(glFrontFace(GL_CCW));
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_C(glUseProgram(0));
	GL_C(glBindTexture(GL_TEXTURE_2D, 0));
	GL_C(glDepthFunc(GL_LESS));

	mat4 viewMatrix = camera.GetViewMatrix();

	dpush("shadows");

	
	dpop(); // shadow

	dpush("gbuffer");
	
	
	GL_C(glViewport(0, 0, fbWidth, fbHeight));
	
	mat4 Vp;

	{
		{
			// update texture indices. used to maintain our history, in TAA
			iHistHdrTex = (iHistHdrTex == 0 ? 1 : 0);

			// keep track of history view and projection matrices for TAA.
			histVp = unjitteredVp;
		}

		// setup matrices, and miscellaneous stuff.
		{
			float ratio = (float)(WINDOW_WIDTH) / (float)WINDOW_HEIGHT;

			mat4 projection = mat4::perspective(0.872665f * 0.5f, (float)(fbWidth) / (float)fbHeight, zNear, zFar);

			// save away the unjittered projection matrix.
			mat4 unjitteredProjection = projection;

			Vp = viewMatrix * projection;
			unjitteredVp = viewMatrix * unjitteredProjection;

			viewMatrix = camera.GetViewMatrix();
		}
	}

	mat4 fullscreenModelMatrix;

	fullscreenModelMatrix.m[1][1] = 0.0f;
	fullscreenModelMatrix.m[2][2] = 0.0f;

	fullscreenModelMatrix.m[2][1] = +1.0f;
	fullscreenModelMatrix.m[1][2] = -1.0f;

	// render to g-buffer
	{
		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		GL_C(glEnable(GL_DEPTH_TEST));
		GL_C(glDepthMask(true));
		GL_C(glFrontFace(GL_CCW));

		GL_C(glUseProgram(geoShader)); // "output geometry to gbuffer" shader
		GL_C(glUniformMatrix4fv(gsViewProjectionMatrixLocation, 1, GL_FALSE, (GLfloat *)Vp.m));
		
		mat4 id;
		GL_C(glUniformMatrix4fv(gsModelMatrixLocation, 1, GL_FALSE, (GLfloat *)id.m));

		GL_C(glUniformMatrix4fv(gsHistVpLocation, 1, GL_FALSE, (GLfloat *)histVp.m));
		GL_C(glUniformMatrix4fv(gsUnjitteredVpLocation, 1, GL_FALSE, (GLfloat *)unjitteredVp.m));

		renderShadowCasters(false);
	}
	
	dpop(); // gbuffer

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


	// shader for rendering geometry.
	// it's basically just a pass-through shader that writes to the g-buffer
	geoShader = LoadNormalShader(
		std::string(R"(
VS_IN_ATTRIB vec3 vertexPosition;
VS_IN_ATTRIB vec3 vertexNormal;

VS_OUT_ATTRIB vec3 fsNormal;
VS_OUT_ATTRIB vec3 fsPos;

VS_OUT_ATTRIB vec4 fsHistCsPos;
VS_OUT_ATTRIB vec4 fsUnjitteredCsPos;

uniform mat4 viewProjectionMatrix;
uniform mat4 modelMatrix;

uniform mat4 uHistVp;
uniform mat4 uUnjitteredVp;

void main()
{
    fsNormal =  (modelMatrix * vec4(vertexNormal, 0.0)).xyz;
    gl_Position = viewProjectionMatrix * modelMatrix * vec4(vertexPosition, 1.0);
    fsPos = (modelMatrix * vec4(vertexPosition, 1.0)).xyz;

    // history clip-space position.
    fsHistCsPos = (uHistVp * vec4(vertexPosition, 1.0));
    // current clip-space pos, but with no jitter.
    fsUnjitteredCsPos = (uUnjitteredVp * vec4(vertexPosition, 1.0));
     
}
)"),


std::string(R"(
FS_IN_ATTRIB vec3 fsNormal;
FS_IN_ATTRIB vec3 fsPos;

FS_IN_ATTRIB vec4 fsHistCsPos;
FS_IN_ATTRIB vec4 fsUnjitteredCsPos;

uniform vec3 meshColor;

DECLARE_GBUFFER_OUTPUT

vec2 outputVelocity(vec4 histCsPos, vec4 curCsPos) {
    vec2 vel = vec2(0.5, 0.5) * ( (histCsPos.xy / histCsPos.w) - (curCsPos.xy / curCsPos.w) ) ;
    return vel;
}

void main()
{
    MGL_FRAG_DATA0 = vec4(meshColor.xyz, 1.0);
    MGL_FRAG_DATA1 = vec4(fsNormal.xyz, 1.0);
    MGL_FRAG_DATA2.xy = outputVelocity(fsHistCsPos, fsUnjitteredCsPos);
}
)")
);
	

	// boxes geometry.
	{
		std::vector<GeoVertex> vertices;
		std::vector<Tri> indices;

	
		for (float x = -5.0f; x < +5.0f; x += 0.9f) {
			for (float y = -5.0f; y < 5.0f; y += 0.9f) {
				
				AddBox(
					x, y, +1.6f,
					0.09f, 0.09, 0.09f,
					vertices, indices);			
			}
		}
	
		boxesMesh.Create(vertices, indices);
	}

	// ground geometry.
	{
		std::vector<GeoVertex> vertices;
		std::vector<Tri> indices;

		AddBox(
			0.0f, 0.0f, 0.0f,
			10.0f, 10.0f, 0.01f,
			vertices, indices);
		
		groundMesh.Create(vertices, indices);
	}
	

	// load all locations from shaders.
	{
		GL_C(gsColorLocation = glGetUniformLocation(geoShader, "meshColor"));
		GL_C(gsViewProjectionMatrixLocation = glGetUniformLocation(geoShader, "viewProjectionMatrix"));
		GL_C(gsModelMatrixLocation = glGetUniformLocation(geoShader, "modelMatrix"));
		GL_C(gsHistVpLocation = glGetUniformLocation(geoShader, "uHistVp"));
		GL_C(gsUnjitteredVpLocation = glGetUniformLocation(geoShader, "uUnjitteredVp"));
		
		
		GL_C(gsPositionAttribLocation = glGetAttribLocation(geoShader, "vertexPosition"));
		GL_C(gsNormalAttribLocation = glGetAttribLocation(geoShader, "vertexNormal"));
	}


	camera = Camera(
		vec3(0, 9.5, 2.0f),
		vec3::normalize(vec3(0.0f, -1.0f, 0.0)));

}
